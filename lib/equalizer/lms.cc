/*
 * Copyright (C) 2015 Bastian Bloessl <bloessl@ccs-labs.org>
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

#include "lms.h"
#include <cstring>
#include <algorithm>
#include <iostream>

using namespace gr::ieee802_11_baofdm::equalizer;

void lms::equalize(gr_complex *in, int n, gr_complex *symbols, uint8_t *bits,
	boost::shared_ptr<gr::digital::constellation> mod, const std::vector<int> &occupied_carriers) {

    // copy over first long sync symbol
	if(n == 0) {
		std::memcpy(d_H, in, 64 * sizeof(gr_complex));

    // copy over second long sync symbol and do some equalization stuff
	} else if(n == 1) {
		double signal = 0;
		double noise = 0;
		for(int i = 0; i < 64; i++) {
			// ignore the nulls and unoccupied carriers
			if((i == 32) || (i < 6) || ( i > 58) || (std::find(occupied_carriers.begin(), occupied_carriers.end(), i) == occupied_carriers.end() && (i != 11) && (i != 25) && (i != 39) && (i != 53))) {
				continue;
			}
			noise += std::pow(std::abs(d_H[i] - in[i]), 2);
			signal += std::pow(std::abs(d_H[i] + in[i]), 2);
			d_H[i] += in[i];
			d_H[i] /= LONG[i] * gr_complex(2, 0);
		}

		d_snr = 10 * std::log10(signal / noise / 2);

    // start copying over data and continue equalization
	} else {
		int c = 0;
		for(int i = 0; i < 64; i++) {
			// 11, 25, 39, 53 are pilots, rest are nulls
			// we need to also ignore the unwanted data carriers here
			if((i == 11) || (i == 25) || (i == 32) || (i == 39) || (i == 53) || (i < 6) || ( i > 58) || (std::find(occupied_carriers.begin(), occupied_carriers.end(), i) == occupied_carriers.end())) {
				continue;
			} else {
				symbols[c] = in[i] / d_H[i];
				bits[c] = mod->decision_maker(&symbols[c]);
				gr_complex point;
				mod->map_to_points(bits[c], &point);
				d_H[i] = gr_complex(1-alpha,0) * d_H[i] + gr_complex(alpha,0) * in[i] / point;
				c++;
			}
		}
	}
}

double
lms::get_snr() {
	return d_snr;
}
