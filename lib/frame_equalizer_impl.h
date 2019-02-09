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

#ifndef INCLUDED_IEEE802_11_FRAME_EQUALIZER_IMPL_H
#define INCLUDED_IEEE802_11_FRAME_EQUALIZER_IMPL_H

#include <ieee802-11/frame_equalizer.h>
#include <ieee802-11/constellations.h>
#include "equalizer/base.h"
#include "viterbi_decoder.h"

namespace gr {
namespace ieee802_11_baofdm {

class frame_equalizer_impl : virtual public frame_equalizer
{

public:
	frame_equalizer_impl(Equalizer algo, double freq, double bw, bool log, bool debug, int num_subcarriers, int num_data_carriers, int num_pilots, const std::vector<std::vector<int>> &occupied_carriers);
	~frame_equalizer_impl();

	void set_algorithm(Equalizer algo);
	void set_bandwidth(double bw);
	void set_frequency(double freq);

	void forecast (int noutput_items, gr_vector_int &ninput_items_required);
	int general_work(int noutput_items,
			gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);

private:


	bool parse_signal(uint8_t *signal);
	bool decode_signal_field(uint8_t *rx_bits);
	void deinterleave(uint8_t *rx_bits);

	equalizer::base *d_equalizer;
	gr::thread::mutex d_mutex;
	std::vector<gr::tag_t> tags;
	bool d_debug;
	bool d_log;
	int  d_current_symbol;
	viterbi_decoder d_decoder;
	const std::vector<std::vector<int>> d_occupied_carriers;

	// added function to perform interleave pattern calculation
	// has no check for numbers that are not mutiples of 3, can add error message later
	void interleave_pattern_calc();

	// freq offset
	double d_freq;  // Hz
	double d_freq_offset_from_synclong;  // Hz, estimation from "sync_long" block
	double d_bw;  // Hz
	double d_er;
	double d_epsilon0;
	// 4 for the number of pilots?
	gr_complex *d_prev_pilots;

	int  d_frame_bytes;
	int  d_frame_symbols;
	int  d_frame_encoding;
	int  d_num_subs;
	int  d_num_data;
	int  d_num_pilots;

	uint8_t *d_deinterleaved;
	gr_complex *symbols;

	boost::shared_ptr<gr::digital::constellation> d_frame_mod;
	constellation_bpsk::sptr d_bpsk;
	constellation_qpsk::sptr d_qpsk;
	constellation_16qam::sptr d_16qam;
	constellation_64qam::sptr d_64qam;

	int *interleaver_pattern;
};

} // namespace ieee802_11_baofdm
} // namespace gr

#endif /* INCLUDED_IEEE802_11_FRAME_EQUALIZER_IMPL_H */
