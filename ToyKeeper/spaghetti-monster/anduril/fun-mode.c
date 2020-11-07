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

PROGMEM const PWM_DATATYPE pattern[] = {0b11111100, 0b01000100, 0b01000000, 0b01001111, 0b11000100, 0b01000000, 0b01001111, 0b11000000, 0b11111100, 0b01001111, 0b11000100, 0b00001111, 0b11000100, 0b11111100, 0b00000000, 0b00000100, 0b11111100, 0b01000100, 0b00000100, 0b01000000, 0b01000100, 0b01001111, 0b11000000, 0b01000000, 0b01000100, 0b01000000, 0b00000000, 0b11111100, 0b11111100, 0b00000100, 0b11111100, 0b00001111, 0b11000000, 0b11111100, 0b00000100, 0b00000100, 0b11111100, 0b01000000, 0b01001111, 0b11000100, 0b11111100, 0b01001111, 0b11000000, 0b01000100, 0b01001111, 0b11000100, 0b11111100, 0b00000000, 0b00000000};
#define PATTERN_SIZE (sizeof(pattern)/sizeof(PWM_DATATYPE))

inline void fun_strobe_iter() {
    uint8_t nominal_level = 
                            #ifdef USE_BIKE_FLASHER_MODE
                            bike_flasher_brightness / 4;
                            #else
                            32;
                            #endif

    // one iteration of main loop()
    for(uint8_t i=0; i<PATTERN_SIZE; i++) {
        uint8_t code = PWM_GET(pattern, i);
	for (uint8_t b = 0; b < 4; b++) {
            set_level(((code & 0xC0) >> 6) * nominal_level);
	    code = code << 2;
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
