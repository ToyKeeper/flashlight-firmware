/*
 * fun-mode.c: Fun mode for Anduril.
 *
 * Copyright (C) 2020 Marshall Flax
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

#ifndef FUN_MODE_C
#define FUN_MODE_C

#include "fun-mode.h"

PROGMEM const PWM_DATATYPE pattern[] = {0b11101010, 0b10001011, 0b10101000, 0b10111000, 0b11101011, 0b10100011, 0b10101110, 0b00000010, 0b11101010, 0b00101000, 0b10101011, 0b10001000, 0b10101000, 0b00001110, 0b11100010, 0b11100011, 0b10001110, 0b00100010, 0b11101000, 0b10111010, 0b11101011, 0b10001010, 0b10111010, 0b11100000};
#define PATTERN_SIZE (sizeof(pattern)/sizeof(PWM_DATATYPE))

inline void fun_strobe_iter() {
    // one iteration of main loop()
    for(uint8_t i=0; i<PATTERN_SIZE; i++) {
        uint8_t code = PWM_GET(pattern, i);
	for (uint8_t b = 0; b < 8; b++) {
            set_level(code & 0x80 ? 
                            #ifdef USE_BIKE_FLASHER_MODE
			    bike_flasher_brightness 
                            #else
                            128
                            #endif
			    : 0);
	    code = code << 1;
	    nice_delay_ms(
                            #ifdef USE_PARTY_STROBE_MODE
			    strobe_delays[party_strobe_e]
                            #else
			    50
                            #endif
			    );
        }
    }
} 
#endif
