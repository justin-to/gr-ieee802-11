 /*
 * Copyright (C) 2013, 2016 Bastian Bloessl <bloessl@ccs-labs.org>
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
#include <ieee802-11/decode_mac.h>

#include "utils.h"
#include "viterbi_decoder.h"

#include <boost/crc.hpp>
#include <gnuradio/io_signature.h>

using namespace gr::ieee802_11;

#define LINKTYPE_IEEE802_11 105 /* http://www.tcpdump.org/linktypes.html */

class decode_mac_impl : public decode_mac {

public:
decode_mac_impl(bool log, bool debug, int num_subcarriers) :
	block("decode_mac",
			gr::io_signature::make(1, 1, num_subcarriers),
			gr::io_signature::make(0, 0, 0)),
	d_log(log),
	d_debug(debug),
	d_snr(0),
	d_nom_freq(0.0),
	d_freq_offset(0.0),
	d_ofdm(BPSK_1_2),
	d_frame(d_ofdm, 0),
	d_frame_complete(true),
	d_num_subs(num_subcarriers){

	// raw pointer to allocated memory
	d_rx_symbols = new uint8_t[MAX_SYM * num_subcarriers];

	message_port_register_out(pmt::mp("out"));
}

// not ideal if program crashes, memory leak
~decode_mac_impl(){
	delete[] d_rx_symbols;
}

int general_work (int noutput_items, gr_vector_int& ninput_items,
		gr_vector_const_void_star& input_items,
		gr_vector_void_star& output_items) {

	// first port within the block
	const uint8_t *in = (const uint8_t*)input_items[0];

	int i = 0;

	std::vector<gr::tag_t> tags;
	const uint64_t nread = this->nitems_read(0);

	dout << "Decode MAC: input " << ninput_items[0] << std::endl;

	while(i < ninput_items[0]) {

		get_tags_in_range(tags, 0, nread + i, nread + i + 1,
			pmt::string_to_symbol("wifi_start"));

		if(tags.size()) {
			if (d_frame_complete == false) {
				dout << "Warning: starting to receive new frame before old frame was complete" << std::endl;
				dout << "Already copied " << copied << " out of " << d_frame.n_sym << " symbols of last frame" << std::endl;
			}
			d_frame_complete = false;

			// polymorphic types carry data from one thread to another, converts from polymorphic type to int, state information data
			pmt::pmt_t dict = tags[0].value;
			int len_data = pmt::to_uint64(pmt::dict_ref(dict, pmt::mp("frame_bytes"), pmt::from_uint64(MAX_PSDU_SIZE+1)));
			int encoding = pmt::to_uint64(pmt::dict_ref(dict, pmt::mp("encoding"), pmt::from_uint64(0)));
			d_snr = pmt::to_double(pmt::dict_ref(dict, pmt::mp("snr"), pmt::from_double(0)));
			d_nom_freq = pmt::to_double(pmt::dict_ref(dict, pmt::mp("freq"), pmt::from_double(0)));
			d_freq_offset = pmt::to_double(pmt::dict_ref(dict, pmt::mp("freq_offset"), pmt::from_double(0)));

			ofdm_param ofdm = ofdm_param((Encoding)encoding);
			frame_param frame = frame_param(ofdm, len_data);

			// check for maximum frame size
			if(frame.n_sym <= MAX_SYM && frame.psdu_size <= MAX_PSDU_SIZE) {
				d_ofdm = ofdm;
				d_frame = frame;
				copied = 0;
				dout << "Decode MAC: frame start -- len " << len_data
					<< "  symbols " << frame.n_sym << "  encoding "
					<< encoding << std::endl;
			} else {
				dout << "Dropping frame which is too large (symbols or bits)" << std::endl;
			}
		}

		// checks for a complete frame 
		if(copied < d_frame.n_sym) {
			dout << "copy one symbol, copied " << copied << " out of " << d_frame.n_sym << std::endl;
			std::memcpy(d_rx_symbols + (copied * d_num_subs), in, d_num_subs);
			copied++;
			if(copied == d_frame.n_sym) {
				dout << "received complete frame - decoding" << std::endl;
				decode();
				in += d_num_subs;
				i++;
				d_frame_complete = true;
				break;
			}
		}

		in += d_num_subs;
		i++;
	}

	consume(0, i);

	return 0;
}

void decode() {

	// n_bpsc stands for number of bits per subcarrier
	// converts symbol stream into a unpacked bit stream
	for(int i = 0; i < d_frame.n_sym * d_num_subs; i++) {
		for(int k = 0; k < d_ofdm.n_bpsc; k++) {
			// double not to guarentee a 1 or 0
			d_rx_bits[i*d_ofdm.n_bpsc + k] = !!(d_rx_symbols[i] & (1 << k));
		}
	}

	deinterleave();
	// likely to be viterbii decoder
	uint8_t *decoded = d_decoder.decode(&d_ofdm, &d_frame, d_deinterleaved_bits);
	descramble(decoded);
	print_output();

	// skip service field
	boost::crc_32_type result;
	result.process_bytes(out_bytes + 2, d_frame.psdu_size);
	// algorithm in Zmodem, should be converted to a string with l2bin(), probably do not need to change
	if(result.checksum() != 558161692) {
		dout << "checksum wrong -- dropping" << std::endl;
		return;
	}

	mylog(boost::format("encoding: %1% - length: %2% - symbols: %3%")
			% d_ofdm.encoding % d_frame.psdu_size % d_frame.n_sym);

	// create protocol data unit
	pmt::pmt_t blob = pmt::make_blob(out_bytes + 2, d_frame.psdu_size - 4);
	pmt::pmt_t enc = pmt::from_uint64(d_ofdm.encoding);
	pmt::pmt_t dict = pmt::make_dict();
	dict = pmt::dict_add(dict, pmt::mp("encoding"), enc);
	dict = pmt::dict_add(dict, pmt::mp("snr"), pmt::from_double(d_snr));
	dict = pmt::dict_add(dict, pmt::mp("nomfreq"), pmt::from_double(d_nom_freq));
	dict = pmt::dict_add(dict, pmt::mp("freqofs"), pmt::from_double(d_freq_offset));
	dict = pmt::dict_add(dict, pmt::mp("dlt"), pmt::from_long(LINKTYPE_IEEE802_11));
	message_port_pub(pmt::mp("out"), pmt::cons(dict, blob));
}

void deinterleave() {

	// n_cbps stands for number of coded bits per subcarrier
	// might need to change this, based on 16
	int n_cbps = d_ofdm.n_cbps;
	int first[n_cbps];
	int second[n_cbps];
	int s = std::max(d_ofdm.n_bpsc / 2, 1);
	// not elegant, can do a modulo later with an error message to ensure user inputs multiple of 3
	int num_rows = d_num_subs / 3;

	// doesn't seem to be unshuffling coded bits within subcarriers, not inverse of second permutation, does not account for interleaving size as seen in the paper
	for(int j = 0; j < n_cbps; j++) {
		first[j] = s * (j / s) + ((j + int(floor(num_rows * j / n_cbps))) % s);
	}

	// doesn't seem to be unscattering adjacent bits into nonadjacent subcarriers
	for(int i = 0; i < n_cbps; i++) {
		second[i] = num_rows * i - (n_cbps - 1) * int(floor(num_rows * i / n_cbps));
	}

	int count = 0;
	// d_rx_bits might be decrypted receive bits or discontinued receive bits
	// loop rearranges each data bit within each symbol 
	for(int i = 0; i < d_frame.n_sym; i++) {
		for(int k = 0; k < n_cbps; k++) {
			d_deinterleaved_bits[i * n_cbps + second[first[k]]] = d_rx_bits[i * n_cbps + k];
		}
	}
}


void descramble (uint8_t *decoded_bits) {
	// possibly for data whitening in DSSS, spreading bits out to increase robustness against localized noise
	int state = 0;
	std::memset(out_bytes, 0, d_frame.psdu_size+2);

	for(int i = 0; i < 7; i++) {
		// packs from most to least significant bit of the decoded bits into state, bits 0 to 6
		if(decoded_bits[i]) {
			state |= 1 << (6 - i);
		}
	}
	out_bytes[0] = state;

	int feedback;
	int bit;

	for(int i = 7; i < d_frame.psdu_size*8+16; i++) {
		feedback = ((!!(state & 64))) ^ (!!(state & 8));
		// returns 1 if the 7th or 4th bit is 1, but not both 
		bit = feedback ^ (decoded_bits[i] & 0x1);
		out_bytes[i/8] |= bit << (i%8);
		// packs from least to most significant bit of the decoded bits
		state = ((state << 1) & 0x7e) | feedback;
		// left shift by 1, removes what was left shifted, preserving bits 1 through 6
	}
}

void print_output() {
	// print for error checking, no need to change 

	dout << std::endl;
	dout << "psdu size" << d_frame.psdu_size << std::endl;
	// why begin at index 2, skipping preamble?
	for(int i = 2; i < d_frame.psdu_size+2; i++) {
		dout << std::setfill('0') << std::setw(2) << std::hex << ((unsigned int)out_bytes[i] & 0xFF) << std::dec << " ";
		// why 15, has to do with short training field?
		if(i % 16 == 15) {
			dout << std::endl;
		}
	}
	dout << std::endl;
	for(int i = 2; i < d_frame.psdu_size+2; i++) {
		// why 127 and 31?
		if((out_bytes[i] > 31) && (out_bytes[i] < 127)) {
			dout << ((char) out_bytes[i]);
		} else {
			dout << ".";
		}
	}
	dout << std::endl;
}

private:
	bool d_debug;
	bool d_log;

	frame_param d_frame;
	ofdm_param d_ofdm;
	double d_snr;  // dB
	double d_nom_freq;  // nominal frequency, Hz
	double d_freq_offset;  // frequency offset, Hz
	int d_num_subs;
	viterbi_decoder d_decoder;

	// memory of d_rx_symbols allocation is changed to variable input by user
	uint8_t *d_rx_symbols;
	uint8_t d_rx_bits[MAX_ENCODED_BITS];
	uint8_t d_deinterleaved_bits[MAX_ENCODED_BITS];
	uint8_t out_bytes[MAX_PSDU_SIZE + 2]; // 2 for signal field

	int copied;
	bool d_frame_complete;
};

decode_mac::sptr
decode_mac::make(bool log, bool debug, int num_subcarriers) {
	return gnuradio::get_initial_sptr(new decode_mac_impl(log, debug, num_subcarriers));
}
