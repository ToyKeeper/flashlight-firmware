/*
 * fsm-ramping.c: Ramping functions for SpaghettiMonster.
 * Handles 1- to 4-channel smooth ramping on a single LED.
 *
 * Copyright (C) 2017 Selene ToyKeeper
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

#ifndef FSM_RAMPING_C
#define FSM_RAMPING_C

#ifdef USE_RAMPING

/* Compressesed ramp tables:

   Most tables start with a run of repeated numbers, then have mostly unique
   numbers, then end with another run of repeated numbers, although the last
   (turbo) value may be different.  So remove the turbo entry from the
   table, and trim repeating numbers from the front and back.  The
   parameters required to expand the array are: the min value, how times it
   repeats, the size of the nonrepeating block, the value of the repeating
   numbers at the end, the turbo value, and the nonrepeating block itself.

   Example: a RAMP_LENGTH=12 table with the following values:

   0, 0, 0, 0, 32, 64, 192, 255, 255, 255, 255, 0 

   compresses to:

   min, min_count, nonrepeat_len, max, turbo, 
   nonrepeating block
   0, 4, 3, 255, 0, 
   32, 64, 192

*/

struct compressed_ramp
{
    uint8_t min;
    uint8_t min_count;
    uint8_t nonrepeat_len;
    uint8_t max;
    uint8_t turbo;
    uint8_t nonrepeat[];
};

uint8_t lookup_pwm(const uint8_t * PROGMEM array, uint8_t offset)
{
#ifdef PWM1_C_LEVELS
    /*
       If n < min_count, return min
       If n < min_count + nonrepeat_len, then lookup the return value and return it
       If n < RAMP_LENGTH-1, return max
       Else, return turbo
    */       
#define p(x) pgm_read_byte(array + offsetof(struct compressed_ramp, x))
    if (offset < p(min_count)) 
        return p(min);
    if (offset < p(min_count) + p(nonrepeat_len))
        return p(nonrepeat[offset - p(min_count)]);
    if (offset < RAMP_LENGTH - 1) 
        return p(max);
    return p(turbo);
#undef p
#else
    return pgm_read_byte(array + offset);
#endif
}

void set_level(uint8_t level) {
    actual_level = level;

    #ifdef USE_SET_LEVEL_GRADUALLY
    gradual_target = level;
    #endif

    #ifdef USE_INDICATOR_LED_WHILE_RAMPING
        #ifdef USE_INDICATOR_LED
        if (! go_to_standby)
            indicator_led((level > 0) + (level > MAX_1x7135));
        #endif
        //if (level > MAX_1x7135) indicator_led(2);
        //else if (level > 0) indicator_led(1);
        //else if (! go_to_standby) indicator_led(0);
    #else
        #if defined(USE_INDICATOR_LED) || defined(USE_AUX_RGB_LEDS)
        if (! go_to_standby) {
            #ifdef USE_INDICATOR_LED
                indicator_led(0);
            #endif
            #ifdef USE_AUX_RGB_LEDS
                rgb_led_set(0);
            #endif
        }
        #endif
    #endif

    //TCCR0A = PHASE;
    if (level == 0) {
        #if PWM_CHANNELS >= 1
        PWM1_LVL = 0;
        #endif
        #if PWM_CHANNELS >= 2
        PWM2_LVL = 0;
        #endif
        #if PWM_CHANNELS >= 3
        PWM3_LVL = 0;
        #endif
        #if PWM_CHANNELS >= 4
        PWM4_LVL = 0;
        #endif
    } else {
        level --;

        #ifdef USE_TINT_RAMPING
        // calculate actual PWM levels based on a single-channel ramp
        // and a global tint value
        uint8_t brightness = lookup_pwm(pwm1_levels, level);
        uint8_t warm_PWM, cool_PWM;

        // auto-tint modes
        uint8_t mytint;
        #if 1
        // perceptual by ramp level
        if (tint == 0) { mytint = 255 * (uint16_t)level / RAMP_SIZE; }
        else if (tint == 255) { mytint = 255 - (255 * (uint16_t)level / RAMP_SIZE); }
        #else
        // linear with power level
        //if (tint == 0) { mytint = brightness; }
        //else if (tint == 255) { mytint = 255 - brightness; }
        #endif
        // stretch 1-254 to fit 0-255 range (hits every value except 98 and 198)
        else { mytint = (tint * 100 / 99) - 1; }

        // middle tints sag, so correct for that effect
        uint16_t base_PWM = brightness;
        // correction is only necessary when PWM is fast
        if (level > HALFSPEED_LEVEL) {
            base_PWM = brightness
                     + ((((uint16_t)brightness) * 26 / 64) * triangle_wave(mytint) / 255);
        }

        cool_PWM = (((uint16_t)mytint * (uint16_t)base_PWM) + 127) / 255;
        warm_PWM = base_PWM - cool_PWM;

        PWM1_LVL = warm_PWM;
        PWM2_LVL = cool_PWM;
        #else

        #if PWM_CHANNELS >= 1
            #if defined(PWM1_TOP)
        // If we have a 16-bit PWM, we can extend our cycle length past 255
        // to get extremely dim at the cost of some flicker.  For the lowest
        // brightness level, make our cycle length 4 times longer.
        if (level == 0)
            PWM1_TOP = 0xFF * 4;
        else
            PWM1_TOP = 0xFF;
            #endif
        PWM1_LVL = lookup_pwm(pwm1_levels, level);
        #endif
        #if PWM_CHANNELS >= 2
        PWM2_LVL = lookup_pwm(pwm2_levels, level);
        #endif
        #if PWM_CHANNELS >= 3
        PWM3_LVL = lookup_pwm(pwm3_levels, level);
        #endif
        #if PWM_CHANNELS >= 4
        PWM4_LVL = lookup_pwm(pwm4_levels, level);
        #endif

        #endif  // ifdef USE_TINT_RAMPING
    }
    #ifdef USE_DYNAMIC_UNDERCLOCKING
    auto_clock_speed();
    #endif
}

#ifdef USE_SET_LEVEL_GRADUALLY
inline void set_level_gradually(uint8_t lvl) {
    gradual_target = lvl;
}

// call this every frame or every few frames to change brightness very smoothly
void gradual_tick() {
    // go by only one ramp level at a time instead of directly to the target
    uint8_t gt = gradual_target;
    if (gt < actual_level) gt = actual_level - 1;
    else if (gt > actual_level) gt = actual_level + 1;

    gt --;  // convert 1-based number to 0-based

    uint8_t target;

    #if PWM_CHANNELS >= 1
    target = lookup_pwm(pwm1_levels, gt);
    if ((gt < actual_level)     // special case for FET-only turbo
            && (PWM1_LVL == 0)  // (bypass adjustment period for first step)
            && (target == 255)) PWM1_LVL = 255;
    else if (PWM1_LVL < target) PWM1_LVL ++;
    else if (PWM1_LVL > target) PWM1_LVL --;
    #endif
    #if PWM_CHANNELS >= 2
    target = lookup_pwm(pwm2_levels, gt);
    if (PWM2_LVL < target) PWM2_LVL ++;
    else if (PWM2_LVL > target) PWM2_LVL --;
    #endif
    #if PWM_CHANNELS >= 3
    target = lookup_pwm(pwm3_levels, gt);
    if (PWM3_LVL < target) PWM3_LVL ++;
    else if (PWM3_LVL > target) PWM3_LVL --;
    #endif
    #if PWM_CHANNELS >= 4
    target = lookup_pwm(pwm4_levels, gt);
    if (PWM4_LVL < target) PWM4_LVL ++;
    else if (PWM4_LVL > target) PWM4_LVL --;
    #endif

    // did we go far enough to hit the next defined ramp level?
    // if so, update the main ramp level tracking var
    if ((PWM1_LVL == lookup_pwm(pwm1_levels, gt))
        #if PWM_CHANNELS >= 2
            && (PWM2_LVL == lookup_pwm(pwm2_levels, gt))
        #endif
        #if PWM_CHANNELS >= 3
            && (PWM3_LVL == lookup_pwm(pwm3_levels, gt))
        #endif
        #if PWM_CHANNELS >= 4
            && (PWM4_LVL == lookup_pwm(pwm4_levels, gt))
        #endif
        )
    {
        actual_level = gt + 1;
    }
}
#endif

#endif  // ifdef USE_RAMPING
#endif
