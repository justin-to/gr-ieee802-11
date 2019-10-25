/*
 * Copyright (C) 2016 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "frame_equalizer_impl.h"
#include "equalizer/base.h"
#include "equalizer/comb.h"
#include "equalizer/lms.h"
#include "equalizer/ls.h"
#include "equalizer/sta.h"
#include "utils.h"
#include <gnuradio/io_signature.h>

#define HEADER_SUBS 48

namespace gr {
namespace ieee802_11_baofdm {

frame_equalizer::sptr
frame_equalizer::make(Equalizer algo, double freq, double bw, bool log, bool debug, int num_subcarriers, int num_data_carriers, int num_pilots, const std::vector<std::vector<int>> &occupied_carriers) {
	return gnuradio::get_initial_sptr
		(new frame_equalizer_impl(algo, freq, bw, log, debug, num_subcarriers, num_data_carriers, num_pilots, occupied_carriers));
}

frame_equalizer_impl::frame_equalizer_impl(Equalizer algo, double freq, double bw, bool log, bool debug, int num_subcarriers, int num_data_carriers, int num_pilots, const std::vector<std::vector<int>> &occupied_carriers) :
	gr::block("frame_equalizer",
			gr::io_signature::make(1, 1, num_subcarriers * sizeof(gr_complex)),
			gr::io_signature::make(1, 1, num_data_carriers)),
	d_current_symbol(0), d_log(log), d_debug(debug), d_equalizer(NULL),
	d_freq(freq), d_bw(bw), d_frame_bytes(0), d_frame_symbols(0),
	d_freq_offset_from_synclong(0.0),
	d_num_subs(num_subcarriers), d_num_data(num_data_carriers),
	d_num_pilots(num_pilots), d_occupied_carriers(occupied_carriers) {

	//raw pointers to allocated memory 
	d_prev_pilots = new gr_complex[d_num_pilots]();
	d_deinterleaved = new uint8_t[48]();

	message_port_register_out(pmt::mp("symbols"));

	// bpsk stands for binary phase shift key ring
	d_bpsk = constellation_bpsk::make();
	d_qpsk = constellation_qpsk::make();
	d_16qam = constellation_16qam::make();
	d_64qam = constellation_64qam::make();

	d_frame_mod = d_bpsk;

	set_tag_propagation_policy(block::TPP_DONT);
	set_algorithm(algo);
}

frame_equalizer_impl::~frame_equalizer_impl() {
	delete[] d_prev_pilots;
	delete[] d_deinterleaved;
}


void
frame_equalizer_impl::set_algorithm(Equalizer algo) {
	gr::thread::scoped_lock lock(d_mutex);
	delete d_equalizer;

	switch(algo) {

	// under library equalizer, needs to change one of these
	// choose the default strategy to change
	// Only LMS has been implemented at this point
	// Does equalization based on pilot subcarriers
	case COMB:
		dout << "Comb" << std::endl;
		d_equalizer = new equalizer::comb();
		break;
	/*
	case LS:
		dout << "LS" << std::endl;
		d_equalizer = new equalizer::ls();
		break;
	*/
	case LMS:
		dout << "LMS" << std::endl;
		d_equalizer = new equalizer::lms();
		break;
	/*
	case STA:
		dout << "STA" << std::endl;
		d_equalizer = new equalizer::sta();
		break;
	*/
	default:
		throw std::runtime_error("Algorithm not implemented");
	}
}

void
frame_equalizer_impl::set_bandwidth(double bw) {
	gr::thread::scoped_lock lock(d_mutex);
	d_bw = bw;
}

void
frame_equalizer_impl::set_frequency(double freq) {
	gr::thread::scoped_lock lock(d_mutex);
	d_freq = freq;
}

void
frame_equalizer_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
	ninput_items_required[0] = noutput_items;
}


// channel estimation and demodulation, guess
// look into frame estimation math
int
frame_equalizer_impl::general_work (int noutput_items,
		gr_vector_int &ninput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items) {

	gr::thread::scoped_lock lock(d_mutex);

	const gr_complex *in = (const gr_complex *) input_items[0];
	uint8_t *out = (uint8_t *) output_items[0];

  if (d_occupied_carriers.empty()) {
    throw std::invalid_argument("Occupied carriers must be of type vector of vector i.e. ((),).");
  }

    // all data carriers, only used when processing headers
    // MAC header is identical for all management frames
    // Octets -> Control Field (2) | Duration of transmission (2) | DA or MAC address destination (6) | SA or MAC address source destination (6) | BSSID or Address of access point (6) | 
    // Sequence Number (2) | Data (0 - 2312) | CRC (4)
    const std::vector<int> standard_carriers = { 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58 };
	int i = 0;
	int o = 0;
    int num_data_subs = HEADER_SUBS;
	gr_complex symbols_header[HEADER_SUBS];
	gr_complex symbols_data[d_num_data];
    const std::vector<int> *used_carriers;

    // current OFDM symbol
	gr_complex current_symbol[d_num_subs];

    memset(symbols_header, 0, sizeof(gr_complex)*HEADER_SUBS);
    memset(symbols_data, 0, sizeof(gr_complex)*d_num_data);
    memset(current_symbol, 0, sizeof(gr_complex)*d_num_subs);

	dout << "FRAME EQUALIZER: input " << ninput_items[0] << "  output " << noutput_items << std::endl;

	while((i < ninput_items[0]) && (o < noutput_items)) {

		get_tags_in_window(tags, 0, i, i + 1, pmt::string_to_symbol("wifi_start"));

		// new frame
		if(tags.size()) {
			d_current_symbol = 0;
			d_frame_symbols = 0;
            // default to bpsk for the header
			d_frame_mod = d_bpsk;

			d_freq_offset_from_synclong = pmt::to_double(tags.front().value) * d_bw / (2 * M_PI);
			d_epsilon0 = pmt::to_double(tags.front().value) * d_bw / (2 * M_PI * d_freq);
			d_er = 0;

			dout << "epsilon: " << d_epsilon0 << std::endl;
		}

		// not interesting -> skip
		if(d_current_symbol > (d_frame_symbols + 2)) {
			i++;
			continue;
		}

        // processing header -> use default numbers
        // preamble contains 12 OFDM symbols to syhcnronize transmitter and receiver. Duration of preamble divided into two parts -> first part sends 10 OFDM traning symbols, second 
        // part starts with a guard interval follow by 2 OFDM traning symbols (last 2 symbols use 52 available usbcarriers with BPSK modulation).
        if(d_current_symbol <= 2) {
            symbols = symbols_header;
            num_data_subs = HEADER_SUBS;
            used_carriers = &standard_carriers;
        }
        // processing data -> use data carrier configuration
        else {
            symbols = symbols_data;
            num_data_subs = d_num_data;
            used_carriers = &d_occupied_carriers[0];
        }

		std::memcpy(current_symbol, in + i*d_num_subs, d_num_subs*sizeof(gr_complex));

		// compensate sampling offset, probably needs change
		// ask about the hardcoded 64 here, probably needs change
		for(int i = 0; i < d_num_subs; i++) {
			current_symbol[i] *= exp(gr_complex(0, 2*M_PI*d_current_symbol*80*(d_epsilon0 + d_er)*(i-32)/64));
		} 
		gr_complex p = equalizer::base::POLARITY[(d_current_symbol - 2) % 127];
		gr_complex sum =
			(current_symbol[11] *  p) +
			(current_symbol[25] *  p) +
			(current_symbol[39] *  p) +
			(current_symbol[53] * -p);

		double beta;
		if(d_current_symbol < 2) {
			beta = arg(
					current_symbol[11] -
					current_symbol[25] +
					current_symbol[39] +
					current_symbol[53]);

		} else {
			beta = arg(
					(current_symbol[11] *  p) +
					(current_symbol[39] *  p) +
					(current_symbol[25] *  p) +
					(current_symbol[53] * -p));
		}

		double er = arg(
				(conj(d_prev_pilots[0]) * current_symbol[11] *  p) +
				(conj(d_prev_pilots[1]) * current_symbol[25] *  p) +
				(conj(d_prev_pilots[2]) * current_symbol[39] *  p) +
				(conj(d_prev_pilots[3]) * current_symbol[53] * -p));

		er *= d_bw / (2 * M_PI * d_freq * 80);

		d_prev_pilots[0] = current_symbol[11] *  p;
		d_prev_pilots[1] = current_symbol[25] *  p;
		d_prev_pilots[2] = current_symbol[39] *  p;
		d_prev_pilots[3] = current_symbol[53] * -p;

		// compensate residual frequency offset
		for(int i = 0; i < d_num_subs; i++) {
			current_symbol[i] *= exp(gr_complex(0, -beta));
		}

		// update estimate of residual frequency offset
        // we don't care about < 2 because this is the preamble (I think)
		if(d_current_symbol >= 2) {

			double alpha = 0.1;
			d_er = (1-alpha) * d_er + alpha * er;
		}

		// do equalization
		d_equalizer->equalize(current_symbol, d_current_symbol,
				symbols, out + o * num_data_subs, d_frame_mod, *used_carriers);

		// signal field (the header)
        // we don't care about < 2 because this is the preamble (I think)
        // currently the header uses all 48 data carriers for interleaving and convolutional coding, whereas the data does NOT
		if(d_current_symbol == 2) {

            // parse the header
			if(decode_signal_field(out + o * HEADER_SUBS)) {

				pmt::pmt_t dict = pmt::make_dict();
				dict = pmt::dict_add(dict, pmt::mp("frame_bytes"), pmt::from_uint64(d_frame_bytes));
                dict = pmt::dict_add(dict, pmt::mp("encoding"), pmt::from_uint64(d_frame_encoding));
				dict = pmt::dict_add(dict, pmt::mp("snr"), pmt::from_double(d_equalizer->get_snr()));
				dict = pmt::dict_add(dict, pmt::mp("freq"), pmt::from_double(d_freq));
				dict = pmt::dict_add(dict, pmt::mp("freq_offset"), pmt::from_double(d_freq_offset_from_synclong));
				add_item_tag(0, nitems_written(0) + o,
						pmt::string_to_symbol("wifi_start"),
						dict,
						pmt::string_to_symbol(alias()));
			}
		}

        // everything after the header is data
        // increment output and emit message when we find data
		if(d_current_symbol > 2) {
			o++;
			pmt::pmt_t pdu = pmt::make_dict();
			message_port_pub(pmt::mp("symbols"), pmt::cons(pmt::make_dict(), pmt::init_c32vector(num_data_subs, symbols)));
		}

		i++;
		d_current_symbol++;
	}

	consume(0, i);
	return o;
}

bool
frame_equalizer_impl::decode_signal_field(uint8_t *rx_bits) {

    // parse the header using a default modulation scheme
	static ofdm_param ofdm(BPSK_1_2_HEADER);
	static frame_param frame(ofdm, 0);

	deinterleave(rx_bits);
	uint8_t *decoded_bits = d_decoder.decode(&ofdm, &frame, d_deinterleaved);

	return parse_signal(decoded_bits);
}

void
frame_equalizer_impl::deinterleave(uint8_t *rx_bits) {
    // header currently uses all 48 data carriers, so we deinterleave it accordingly
	for(int i = 0; i < HEADER_SUBS; i++) {
		d_deinterleaved[i] = rx_bits[interleaver_pattern[i]];
	}
}

// unpacks header fields that are necessary for decoding/demoding data
bool
frame_equalizer_impl::parse_signal(uint8_t *decoded_bits) {

	int r = 0;
	d_frame_bytes = 0;
	bool parity = false;
	for(int i = 0; i < 17; i++) {
		parity ^= decoded_bits[i];
		if((i < 4) && decoded_bits[i]) {
			r = r | (1 << i);
		}

		if(decoded_bits[i] && (i > 4) && (i < 17)) {
			d_frame_bytes = d_frame_bytes | (1 << (i-5));
		}
	}

	if(parity != decoded_bits[17]) {
		dout << "SIGNAL: wrong parity" << std::endl;
		return false;
	}

    // NEED TO MODIFY THIS WHEN WE CHANGE OFDM_PARAM
	switch(r) {
	case 11:
		d_frame_encoding = 0;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 12); // divide by 12, not 24, when using 24 subcarriers
		d_frame_mod = d_bpsk;
		dout << "Encoding: 3 Mbit/s   ";
		break;
	case 15:
		d_frame_encoding = 1;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 36);
		d_frame_mod = d_bpsk;
		dout << "Encoding: 4.5 Mbit/s   ";
		break;
	case 10:
		d_frame_encoding = 2;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 48);
		d_frame_mod = d_qpsk;
		dout << "Encoding: 6 Mbit/s   ";
		break;
	case 14:
		d_frame_encoding = 3;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 72);
		d_frame_mod = d_qpsk;
		dout << "Encoding: 9 Mbit/s   ";
		break;
	case 9:
		d_frame_encoding = 4;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 96);
		d_frame_mod = d_16qam;
		dout << "Encoding: 12 Mbit/s   ";
		break;
	case 13:
		d_frame_encoding = 5;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 144);
		d_frame_mod = d_16qam;
		dout << "Encoding: 18 Mbit/s   ";
		break;
	case 8:
		d_frame_encoding = 6;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 192);
		d_frame_mod = d_64qam;
		dout << "Encoding: 24 Mbit/s   ";
		break;
	case 12:
		d_frame_encoding = 7;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 216);
		d_frame_mod = d_64qam;
		dout << "Encoding: 27 Mbit/s   ";
		break;
	default:
		dout << "unknown encoding" << std::endl;
		return false;
	}

	mylog(boost::format("encoding: %1% - length: %2% - symbols: %3%")
			% d_frame_encoding % d_frame_bytes % d_frame_symbols);
	return true;
}


const int
frame_equalizer_impl::interleaver_pattern[48] = {
	 0, 3, 6, 9,12,15,18,21,
	24,27,30,33,36,39,42,45,
	 1, 4, 7,10,13,16,19,22,
	25,28,31,34,37,40,43,46,
	 2, 5, 8,11,14,17,20,23,
	26,29,32,35,38,41,44,47};

} /* namespace ieee802_11_baofdm */
} /* namespace gr */
