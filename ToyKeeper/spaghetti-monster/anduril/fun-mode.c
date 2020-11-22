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

PROGMEM const PWM_DATATYPE pattern[] = {0b11111100, 0b10001000, 0b10000000, 0b10001111, 0b11001000, 0b10000000, 0b10001111, 0b11000000, 0b11111100, 0b10001111, 0b11001000, 0b00001111, 0b11001000, 0b11111100, 0b00000000, 0b00001000, 0b11111100, 0b10001000, 0b00001000, 0b10000000, 0b10001000, 0b10001111, 0b11000000, 0b10000000, 0b10001000, 0b10000000, 0b00000000, 0b11111100, 0b11111100, 0b00001000, 0b11111100, 0b00001111, 0b11000000, 0b11111100, 0b00001000, 0b00001000, 0b11111100, 0b10000000, 0b10001111, 0b11001000, 0b11111100, 0b10001111, 0b11000000, 0b10001000, 0b10001111, 0b11001000, 0b11111100, 0b00000000, 0b00000000, 0b00000000};
#define PATTERN_SIZE (sizeof(pattern)/sizeof(PWM_DATATYPE))

inline void fun_strobe_iter() {
    const int16_t nominal_level = 
                            #ifdef USE_BIKE_FLASHER_MODE
                            clamp16(bike_flasher_brightness, 
                                    #if defined(USE_AUX_RGB_LEDS) 
                                    0
                                    #else
                                    1
                                    #endif
                                    , STROBE_BRIGHTNESS/4);
                            #else
                            1;
                            #endif

    // one iteration of main loop()
    uint8_t prior_brightness = 0;
    for(uint8_t i=0; i<PATTERN_SIZE; i++) {
        uint8_t code = PWM_GET(pattern, i);

        // four semi-nibbles per byte
        for (uint8_t b = 0; b < 4; b++) {
          uint8_t brightness = (code & 0xC0) >> 6;
          if (brightness != prior_brightness) {
              set_level(brightness * nominal_level);
              #if defined(USE_AUX_RGB_LEDS) 
              switch (brightness) {
                case 1:  rgb_led_set(0x01 << 1); break;
                case 2:  rgb_led_set(0x11 << 1); break;
                case 3:  rgb_led_set(0x14 << 1); break;
                default: rgb_led_set(0); break;
              }
              #endif
              prior_brightness = brightness;
          }
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
    set_level(0);
    #if defined(USE_AUX_RGB_LEDS) 
    rgb_led_set(0);
    #endif
} 
#endif
