/*
 * "BistroSN" firmware
 * This code runs on Convoy 7135 (1x7135 + 7x7135) driver with 
 * Sonix SN8F5xxxx MCU and a capacitor to measure offtime (OTC).
 *
 * Copyright (C) 2015 Selene ToyKeeper (original ATtiny version)
 * Copyright (C) 2025 SilicaGel (SN8F5xxx port)
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
 *
 *
 * Sonix SN8F5701 Diagram
 *                 ----
 *           VCC -|1  8|- GND
 *  PWM (1x7135) -|2  7|- OTC
 *  PWM (7x7135) -|3  6|- SWAT (ISP pin)
 *               -|4  5|- Temperature ADC
 *                 ----
 *
 * CALIBRATION
 *
 *   To find out what values to use, uncomment `VOLTAGE_TEST` define, set 
 *   `mode_idx` to `VOLTAGE_TEST`, set `mode_override` to `1`, compile, flash
 *   and hook the light up to each voltage you need a value for.  This is
 *   much more reliable than attempting to calculate the values from a
 *   theoretical formula.
 *
 *   Same for off-time capacitor values.  Measure, don't guess.
 */
// Choose your MCU here, or in the build script
#ifndef MCU_DEFINED
#define MCU_SN8F5701
#define MCU_DEFINED
#endif
// Specify an I/O pin layout
#ifndef LAYOUT_DEFINED
#define LAYOUT_CONVOY_7135
#define LAYOUT_DEFINED
#endif
// Also, assign I/O pins in this file:
#include "tk-sn8f5xxx.h"

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON         // Comment out to disable LVP

#ifndef LAYOUT_NO_OFFTIM3
#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long
#endif

// scripts/level_calc.py 2 64 7135 3 0.25 140 7135 3 1.75 980
#define RAMP_SIZE  64
// x**3 curve
#define RAMP_1X7135  3,4,4,5,6,8,10,12,15,18,22,26,31,36,42,49,57,66,75,85,96,108,121,136,151,167,185,203,223,245,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
#define RAMP_7X7135  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,8,12,15,20,24,28,33,38,43,49,54,60,66,73,80,87,94,102,109,118,126,135,144,154,164,174,184,195,206,218,230,242,255

// uncomment to ramp up/down to a mode instead of jumping directly
#define SOFT_START

// Enable battery indicator mode?
#define USE_BATTCHECK
// Choose a battery indicator style
//#define BATTCHECK_4bars  // up to 4 blinks
//#define BATTCHECK_8bars  // up to 8 blinks
#define BATTCHECK_VpT  // Volts + tenths

// output to use for blinks on battery check (and other modes)
#define BLINK_BRIGHTNESS    RAMP_SIZE/4
// ms per normal-speed blink
#define BLINK_SPEED         1000

// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .
#define HIDDENMODES         BIKING_STROBE,BATTCHECK,POLICE_STROBE,TURBO

#define TURBO     RAMP_SIZE       // Convenience code for turbo mode
#define BATTCHECK 254       // Convenience code for battery check mode
#define GROUP_SELECT_MODE 253
#define TEMP_CAL_MODE 252
// Uncomment to enable tactical strobe mode
//#define STROBE    251       // Convenience code for strobe mode
// Uncomment to unable a 2-level stutter beacon instead of a tactical strobe
#define BIKING_STROBE 250   // Convenience code for biking strobe mode
// comment out to use minimal version instead (smaller)
#define FULL_BIKING_STROBE
//#define RAMP 249       // ramp test mode for tweaking ramp shape
#define POLICE_STROBE 248
//#define RANDOM_STROBE 247
//#define SOS 246
//#define DELAY_TEST 245 // test mode to calibrate software delay
//#define TEMP_TEST 244 // test mode to calibrate tempreature ADC
//#define VOLTAGE_TEST 243 // test mode to calibrate voltage ADC
//#define OFFTIM_TEST 242 // test mode to calibrate OTC capacitor

// thermal step-down
#define TEMPERATURE_MON

// Calibrate voltage and OTC in this file:
#include "tk-calibration.h"

/*
 * =========================================================================
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_MS        // Also use _delay_ms()
#define USE_DELAY_S         // Also use _delay_s()
#include "tk-delay.h"

#include "tk-voltage.h"

#ifdef RANDOM_STROBE
#include "tk-random.h"
#endif

/*
 * global variables
 */

// Config option variables
#define USE_FIRSTBOOT
#ifdef USE_FIRSTBOOT
#define FIRSTBOOT 0b01010101
uint8_t firstboot = FIRSTBOOT;  // detect initial boot or factory reset
#endif
uint8_t modegroup = 5;     // which mode group (set above in #defines)
uint8_t enable_moon = 1;   // Should we add moon to the set of modes?
uint8_t reverse_modes = 0; // flip the mode order?
uint8_t memory = 0;        // mode memory, or not (set via soldered star)
#ifdef OFFTIM3
uint8_t offtim3 = 1;       // enable medium-press?
#endif
#ifdef TEMPERATURE_MON
uint8_t maxtemp = 195;      // temperature step-down threshold
#endif
uint8_t muggle_mode = 0;   // simple mode designed for muggles
// Other state variables
uint8_t mode_override = 0; // do we need to enter a special mode?
uint8_t mode_idx = 0;      // current or last-used mode number
uint8_t eepos = 0;
// Counter for entering config mode.
// Unlike the original Bistro firmware, this port does not rely on RAM
// persistence to remember the variable. Instead, the value is saved to
// EEPROM together with mode index.
uint8_t fast_presses = 0;

// total length of current mode group's array
uint8_t mode_cnt;
// number of regular non-hidden modes in current mode group
uint8_t solid_modes;
// number of hidden modes in the current mode group
// (hardcoded because both groups have the same hidden modes)
//uint8_t hidden_modes = NUM_HIDDEN;  // this is never used


__code const uint8_t hiddenmodes[] = { HIDDENMODES };
// default values calculated by group_calc.py
// Each group must be 8 values long, but can be cut short with a zero.
#define NUM_MODEGROUPS 9  // don't count muggle mode
__code const uint8_t modegroups[] = {
    64,  0,  0,  0,  0,  0,  0,  0,
    11, 64,  0,  0,  0,  0,  0,  0,
    11, 35, 64,  0,  0,  0,  0,  0,
    11, 26, 46, 64,  0,  0,  0,  0,
    11, 23, 36, 50, 64,  0,  0,  0,
    11, 20, 31, 41, 53, 64,  0,  0,
    29, 64,POLICE_STROBE,0,0,0,0,0,  // 7: special group A
    BIKING_STROBE,BATTCHECK,11,29,64,0,0,0,  // 8: special group B
     9, 18, 29, 46, 64,  0,  0,  0,  // 9: special group C
    11, 29, 50,  0,                  // muggle mode, exception to "must be 8 bytes long"
};
//uint8_t modes[] = { 1,2,3,4,5,6,7,8,9, HIDDENMODES };  // make sure this is long enough...
uint8_t modes[9 + sizeof(hiddenmodes)];  // make sure this is long enough...

// Modes (gets set when the light starts up based on saved config values)
__code const uint8_t ramp_1x7135[] = { RAMP_1X7135 };
__code const uint8_t ramp_7x7135[] = { RAMP_7X7135 };

void save_mode(void) {  // save the current mode index (with wear leveling)
    uint8_t oldpos=eepos;

    eepos = (eepos+2) & ((EEPSIZE/2)-1);  // wear leveling, use next cell

    eeprom_write_byte(eepos, mode_idx);  // save current state
    eeprom_write_byte(eepos+1, fast_presses);  // save fast presses count
    eeprom_write_byte(oldpos, 0xff);     // erase old state
    eeprom_write_byte(oldpos+1, 0xff);     // erase old state
}

#define OPT_firstboot (EEPSIZE-1)
#define OPT_modegroup (EEPSIZE-2)
#define OPT_memory (EEPSIZE-3)
#define OPT_offtim3 (EEPSIZE-4)
#define OPT_maxtemp (EEPSIZE-5)
#define OPT_mode_override (EEPSIZE-6)
#define OPT_moon (EEPSIZE-7)
#define OPT_revmodes (EEPSIZE-8)
#define OPT_muggle (EEPSIZE-9)
void save_state(void) {  // central method for writing complete state
    eeprom_write_byte(OPT_modegroup, modegroup);
    eeprom_write_byte(OPT_memory, memory);
#ifdef OFFTIM3
    eeprom_write_byte(OPT_offtim3, offtim3);
#endif
#ifdef TEMPERATURE_MON
    eeprom_write_byte(OPT_maxtemp, maxtemp);
#endif
    eeprom_write_byte(OPT_mode_override, mode_override);
    eeprom_write_byte(OPT_moon, enable_moon);
    eeprom_write_byte(OPT_revmodes, reverse_modes);
    eeprom_write_byte(OPT_muggle, muggle_mode);
#ifdef USE_FIRSTBOOT
    eeprom_write_byte(OPT_firstboot, firstboot);
#endif

    save_mode();
}

#ifndef USE_FIRSTBOOT
inline void reset_state() {
    mode_idx = 0;
    modegroup = 5;
    save_state();
}
#endif

void restore_state(void) {
    uint8_t eep;

#ifdef USE_FIRSTBOOT
    // check if this is the first time we have powered on
    eep = eeprom_read_byte(OPT_firstboot);
    if (eep != FIRSTBOOT) {
        // not much to do; the defaults should already be set
        // while defining the variables above
        save_state();
        return;
    }
#else
    uint8_t first = 1;
#endif

    // find the mode index data
    for(eepos=0; eepos<(EEPSIZE/2); eepos+=2) {
        eep = eeprom_read_byte(eepos);
        if (eep != 0xff) {
            mode_idx = eep;
            fast_presses = eeprom_read_byte(eepos+1);
#ifndef USE_FIRSTBOOT
            first = 0;
#endif
            break;
        }
    }
#ifndef USE_FIRSTBOOT
    // if no mode_idx was found, assume this is the first boot
    if (first) {
        reset_state();
        return;
    }
#endif

    // load other config values
    modegroup = eeprom_read_byte(OPT_modegroup);
    memory    = eeprom_read_byte(OPT_memory);
#ifdef OFFTIM3
    offtim3   = eeprom_read_byte(OPT_offtim3);
#endif
#ifdef TEMPERATURE_MON
    maxtemp   = eeprom_read_byte(OPT_maxtemp);
#endif
    mode_override = eeprom_read_byte(OPT_mode_override);
    enable_moon   = eeprom_read_byte(OPT_moon);
    reverse_modes = eeprom_read_byte(OPT_revmodes);
    muggle_mode   = eeprom_read_byte(OPT_muggle);

    // unnecessary, save_state handles wrap-around
    // (and we don't really care about it skipping cell 0 once in a while)
    //else eepos=0;

#ifndef USE_FIRSTBOOT
    if (modegroup >= NUM_MODEGROUPS) reset_state();
#endif
}

inline void next_mode(void) {
    mode_idx += 1;
    if (mode_idx >= solid_modes) {
        // Wrap around, skipping the hidden modes
        // (note: this also applies when going "forward" from any hidden mode)
        // FIXME? Allow this to cycle through hidden modes?
        mode_idx = 0;
    }
}

#ifdef OFFTIM3
inline void prev_mode(void) {
    // simple mode has no reverse
    if (muggle_mode) { return next_mode(); }

    if (mode_idx == solid_modes) {
        // If we hit the end of the hidden modes, go back to moon
        mode_idx = 0;
    } else if (mode_idx > 0) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= 1;
    } else {
        // Otherwise, wrap around (this allows entering hidden modes)
        mode_idx = mode_cnt - 1;
    }
}
#endif

void count_modes(void) {
    /*
     * Determine how many solid and hidden modes we have.
     *
     * (this matters because we have more than one set of modes to choose
     *  from, so we need to count at runtime)
     */
    // copy config to local vars to avoid accidentally overwriting them in muggle mode
    // (also, it seems to reduce overall program size)
    uint8_t my_modegroup = modegroup;
    uint8_t my_enable_moon = enable_moon;
    uint8_t my_reverse_modes = reverse_modes;

    // override config if we're in simple mode
    if (muggle_mode) {
        my_modegroup = NUM_MODEGROUPS;
        my_enable_moon = 0;
        my_reverse_modes = 0;
    }

    uint8_t *dest;
    const __code uint8_t *src = modegroups + (my_modegroup<<3);
    dest = modes;

    // add moon mode (or not) if config says to add it
    if (my_enable_moon) {
        modes[0] = 1;
        dest ++;
    }

    // Figure out how many modes are in this group
    //solid_modes = modegroup + 1;  // Assume group N has N modes
    // No, how about actually counting the modes instead?
    // (in case anyone changes the mode groups above so they don't form a triangle)
    for(solid_modes=0;
        (solid_modes<8) && *src;
        solid_modes++, src++ )
    {
        *dest++ = *src;
    }

    // add regular modes
    //memcpy_P(dest, src, solid_modes);  // was already copied above
    // add hidden modes
    //memcpy_P(dest + solid_modes, hiddenmodes, sizeof(hiddenmodes));
    // smaller than memcpy_p()
    for( src=hiddenmodes; src<hiddenmodes+sizeof(hiddenmodes); src++ )
    {
        *dest++ = *src;
    }
    // final count
#ifdef OFFTIM3
    mode_cnt = solid_modes + sizeof(hiddenmodes);
#endif
    if (my_reverse_modes) {
        // TODO: yuck, isn't there a better way to do this?
        uint8_t i;
        src = modegroups + (my_modegroup<<3) + solid_modes;
        dest = modes;
        for(i=0; i<solid_modes; i++) {
            src --;
            *dest = *src;
            dest ++;
        }
        if (my_enable_moon) {
            *dest = 1;
        }
        mode_cnt --;  // get rid of last hidden mode, since it's a duplicate turbo
    }
    if (my_enable_moon) {
        mode_cnt ++;
        solid_modes ++;
    }
}

#ifdef ALT_PWM_LVL
inline void set_output(uint8_t pwm1, uint8_t pwm2) {
#else
inline void set_output(uint8_t pwm1) {
#endif
    PWM_LVL = pwm1;
    #ifdef ALT_PWM_LVL
    ALT_PWM_LVL = pwm2;
    #endif
}

void set_level(uint8_t level) {
    if (level == 0) {
        set_output(0,0);
    } else {
        level -= 1;
        set_output(ramp_7x7135[level],
                   ramp_1x7135[level]);
    }
}

#ifdef SOFT_START
void set_mode(uint8_t mode) {
    static uint8_t actual_level = 0;
    uint8_t target_level = mode;
    int8_t shift_amount;
    int8_t diff;
    do {
        diff = target_level - actual_level;
        shift_amount = (diff >> 2) | (diff!=0);
        actual_level += shift_amount;
        set_level(actual_level);
        //_delay_ms(RAMP_SIZE/20);  // slow ramp
        _delay_ms(RAMP_SIZE/4);  // fast ramp
    } while (target_level != actual_level);
}
#else
#define set_mode set_level
    //set_level(mode);
#endif  // SOFT_START

void blink(uint8_t val, uint16_t speed)
{
    for (; val>0; val--)
    {
        set_level(BLINK_BRIGHTNESS);
        _delay_ms(speed);
        set_level(0);
        _delay_ms(speed);
        _delay_ms(speed);
    }
}

inline void strobe(uint8_t ontime, uint8_t offtime) {
    uint8_t i;
    for(i=0;i<8;i++) {
        set_level(RAMP_SIZE);
        _delay_ms(ontime);
        set_level(0);
        _delay_ms(offtime);
    }
}

#ifdef SOS
inline void SOS_mode() {
#define SOS_SPEED 200
    blink(3, SOS_SPEED);
    _delay_ms(SOS_SPEED*5);
    blink(3, SOS_SPEED*5/2);
    //_delay_ms(SOS_SPEED);
    blink(3, SOS_SPEED);
    _delay_s(); _delay_s();
}
#endif

void toggle(uint8_t *var, uint8_t num) {
    // Used for config mode
    // Changes the value of a config option, waits for the user to "save"
    // by turning the light off, then changes the value back in case they
    // didn't save.  Can be used repeatedly on different options, allowing
    // the user to change and save only one at a time.
    blink(num, BLINK_SPEED/8);  // indicate which option number this is
    *var ^= 1;
    save_state();
    // "buzz" for a while to indicate the active toggle window
    blink(32, 500/32);
    /*
    for(uint8_t i=0; i<32; i++) {
        set_level(BLINK_BRIGHTNESS * 3 / 4);
        _delay_ms(20);
        set_level(0);
        _delay_ms(20);
    }
    */
    // if the user didn't click, reset the value and return
    *var ^= 1;
    save_state();
    _delay_s();
}

#ifdef TEMPERATURE_MON
uint8_t get_temp(void) {
    ADC_on_temperature();
    // average a few values; temperature is noisy
    uint16_t temp = 0;
    uint8_t i;
    get_temperature();
    for(i=0; i<16; i++) {
        temp += get_temperature();
        _delay_ms(5);
    }
    temp >>= 4;
    return temp;
}
#endif  // TEMPERATURE_MON

inline uint16_t read_otc(void) {
    // Read and return the off-time cap value
    // Start up ADC for capacitor pin
    // disable digital input on ADC pin to reduce power consumption
    P0CON |= (1 << CAP_PIN); // set AIN5/P05 pin's mode at pure analog pin
    P0M &= ~(1 << CAP_PIN); // AIN5/P05 input mode
    P0UR &= ~(1 << CAP_PIN); // AIN5/P05 disable pull-high
    ADM &= ~(1 << 5); // Clear EOC bit
    ADM = (uint8_t)(1 << 7) | (uint8_t)(CAP_CHANNEL << 0); // enable ADC and set channel
    ADR = (1 << 6) | (2 << 4); // enable ADC channel and set speed to fosc/1
    VREFH = 0; // internal reference 2.0V;
    ADM |= (1 << 6); // start ADC conversion
    while (!(ADM & (1 << 5))) {} // Wait for conversion
    return ((uint16_t)ADB) << 4 | (ADR & 0x0F); // Return 12-bit result
}

void reset_fast_presses(void) {
    if (fast_presses) {
        fast_presses = 0;
        save_mode();
    }
}

#if defined(TEMP_TEST) || defined(VOLTAGE_TEST) || defined(OFFTIM_TEST)
#define TEST_MAX_DIGITS 5
void debug_out(uint16_t value, uint8_t digit_count) {
    uint8_t digits[TEST_MAX_DIGITS];
    if (digit_count > TEST_MAX_DIGITS) digit_count = TEST_MAX_DIGITS;
    blink(3,20);
    _delay_ms(BLINK_SPEED);
    for (uint8_t i = 0; i < digit_count; i++) {
        digits[digit_count - 1 - i] = value % 10;
        value /= 10;
    }
    for (uint8_t i = 0; i < digit_count; i++) {
        blink(digits[i], BLINK_SPEED/4);
        _delay_ms(BLINK_SPEED);
        blink(1,5);
        _delay_ms(BLINK_SPEED*3/2);
    }
    _delay_s(); _delay_s();
}
#endif // defined(TEMP_TEST) || defined(VOLTAGE_TEST) || defined(OFFTIM_TEST)

int main(void)
{
    // init system clock
    CLKSEL = 0x05; // fcpu = fosc / 4
    CLKCMD = 0x69; // apply fcpu
    CKCON = 0x00; // IROM fetch = fcpu / 1

    // check the OTC immediately before it has a chance to charge or discharge
    uint16_t cap_val = read_otc();  // save it for later

    // Set PWM pin to output
    P0 &= ~(1 << PWM_PIN);     // set output to 0
    P0M |= (1 << PWM_PIN);     // enable main channel
    #ifdef ALT_PWM_PIN
    P0 &= ~(1 << ALT_PWM_PIN);     // set output to 0
    P0M |= (1 << ALT_PWM_PIN); // enable second channel
    #endif

    // Set timer to do PWM for correct output pin and set prescaler timing
    T3M |= 4 << 4; // Frequency of Timer 3 is fosc / 8
    T3Y = 0x00FF; // PWM period setting
    PWCH |= (1 << 0); // Enable channel 0
    PWCH |= (1 << 1); // Enable channel 1
    T3M |= (1 << 7); // Enable Timer 3

    // Read config values and saved state
    restore_state();

    // Enable the current mode group
    count_modes();


    // TODO: Enable this?  (might prevent some corner cases, but requires extra room)
    // memory decayed, reset it
    // (should happen on med/long press instead
    //  because mem decay is *much* slower when the OTC is charged
    //  so let's not wait until it decays to reset it)
    //if (fast_presses > 0x20) { fast_presses = 0; }

    // check button press time, unless the mode is overridden
    if (! mode_override) {
        if (cap_val > CAP_SHORT) {
            // Indicates they did a short press, go to the next mode
            // We don't care what the fast_presses value is as long as it's over 15
            fast_presses = (fast_presses+1) & 0x1f;
            next_mode(); // Will handle wrap arounds
#ifdef OFFTIM3
        } else if (cap_val > CAP_MED) {
            // User did a medium press, go back one mode
            fast_presses = 0;
            if (offtim3) {
                prev_mode();  // Will handle "negative" modes and wrap-arounds
            } else {
                next_mode();  // disabled-med-press acts like short-press
                              // (except that fast_presses isn't reliable then)
            }
#endif
        } else {
            // Long press, keep the same mode
            // ... or reset to the first mode
            fast_presses = 0;
            if (muggle_mode  || (! memory)) {
                // Reset to the first mode
                mode_idx = 0;
            }
        }
    }
    save_mode();

    #ifdef CAP_PIN
    // Charge up the capacitor by setting CAP_PIN to output
    ADR &= ~(1 << 6); // disable ADC channel
    P0CON &= ~(1 << CAP_PIN); // set CAP_PIN to digital+analog mode
    P0 |= (1 << CAP_PIN); // set CAP_PIN high
    P0M |= 1 << CAP_PIN; // set CAP_PIN to output
    #endif

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif

    uint8_t output;
    uint8_t actual_level;
    uint8_t i = 0;
#ifdef TEMPERATURE_MON
    uint8_t overheat_count = 0;
#endif
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t voltage;
    // Make sure voltage reading is running for later
    ADM |= (1 << 6);
#endif
    output = modes[mode_idx];
    actual_level = output;
    // handle mode overrides, like mode group selection and temperature calibration
    if (mode_override) {
        // do nothing; mode is already set
        //mode_idx = mode_override;
        reset_fast_presses();
        output = mode_idx;
    }
    while(1) {
        if (fast_presses > 0x0f) {  // Config mode
            _delay_s();       // wait for user to stop fast-pressing button
            reset_fast_presses(); // exit this mode after one use
            mode_idx = 0;

            // Enter or leave "muggle mode"?
            toggle(&muggle_mode, 1);
            if (muggle_mode) { continue; };  // don't offer other options in muggle mode

            toggle(&memory, 2);

            toggle(&enable_moon, 3);

            toggle(&reverse_modes, 4);

            // Enter the mode group selection mode?
            mode_idx = GROUP_SELECT_MODE;
            toggle(&mode_override, 5);
            mode_idx = 0;

#ifdef OFFTIM3
            toggle(&offtim3, 6);
#endif

#ifdef TEMPERATURE_MON
            // Enter temperature calibration mode?
            mode_idx = TEMP_CAL_MODE;
            toggle(&mode_override, 7);
            mode_idx = 0;
#endif

            #ifdef USE_FIRSTBOOT
            toggle(&firstboot, 8);
            #endif

            output = modes[mode_idx];
            actual_level = output;
        }
#ifdef STROBE
        else if (output == STROBE) {
            // 10Hz tactical strobe
            strobe(33,67);
        }
#endif // ifdef STROBE
#ifdef POLICE_STROBE
        else if (output == POLICE_STROBE) {
            // police-like strobe
            //for(i=0;i<8;i++) {
                strobe(20,40);
            //}
            //for(i=0;i<8;i++) {
                strobe(40,80);
            //}
        }
#endif // ifdef POLICE_STROBE
#ifdef RANDOM_STROBE
        else if (output == RANDOM_STROBE) {
            // pseudo-random strobe
            uint8_t ms = 34 + (pgm_rand() & 0x3f);
            strobe(ms, ms);
            //strobe(ms, ms);
        }
#endif // ifdef RANDOM_STROBE
#ifdef BIKING_STROBE
        else if (output == BIKING_STROBE) {
            // 2-level stutter beacon for biking and such
#ifdef FULL_BIKING_STROBE
            // normal version
            for(i=0;i<4;i++) {
                set_output(255,0);
                _delay_ms(5);
                set_output(0,255);
                _delay_ms(65);
            }
            _delay_ms(720);
#else
            // small/minimal version
            set_output(255,0);
            _delay_ms(10);
            set_output(0,255);
            _delay_s();
#endif
        }
#endif  // ifdef BIKING_STROBE
#ifdef SOS
        else if (output == SOS) { SOS_mode(); }
#endif // ifdef SOS
#ifdef RAMP
        else if (output == RAMP) {
            int8_t r;
            // simple ramping test
            for(r=1; r<=RAMP_SIZE; r++) {
                set_level(r);
                _delay_ms(25);
            }
            for(r=RAMP_SIZE; r>0; r--) {
                set_level(r);
                _delay_ms(25);
            }
        }
#endif  // ifdef RAMP
#ifdef USE_BATTCHECK
        else if (output == BATTCHECK) {
#ifdef BATTCHECK_VpT
            // blink out volts and tenths
            _delay_ms(100);
            uint8_t result = battcheck();
            blink(result >> 5, BLINK_SPEED/8);
            _delay_ms(BLINK_SPEED);
            blink(1,5);
            _delay_ms(BLINK_SPEED*3/2);
            blink(result & 0b00011111, BLINK_SPEED/8);
#else  // ifdef BATTCHECK_VpT
            // blink zero to five times to show voltage
            // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
            blink(battcheck(), BLINK_SPEED/8);
#endif  // ifdef BATTCHECK_VpT
            // wait between readouts
            _delay_s(); _delay_s();
        }
#endif // ifdef USE_BATTCHECK
        else if (output == GROUP_SELECT_MODE) {
            // exit this mode after one use
            mode_idx = 0;
            mode_override = 0;

            for(i=0; i<NUM_MODEGROUPS; i++) {
                modegroup = i;
                save_state();

                blink(1, BLINK_SPEED/3);
            }
            _delay_s(); _delay_s();
        }
#ifdef TEMP_CAL_MODE
        else if (output == TEMP_CAL_MODE) {
            // make sure we don't stay in this mode after button press
            mode_idx = 0;
            mode_override = 0;

            // Allow the user to turn off thermal regulation if they want
            maxtemp = 255;
            save_state();
            set_mode(RAMP_SIZE/4);  // start somewhat dim during turn-off-regulation mode
            _delay_s(); _delay_s();

            // run at highest output level, to generate heat
            set_mode(RAMP_SIZE);

            // measure, save, wait...  repeat
            while(1) {
                maxtemp = get_temp();
                save_state();
                _delay_s(); _delay_s();
            }
        }
#endif  // TEMP_CAL_MODE
#ifdef DELAY_TEST
        else if (output == DELAY_TEST) {
            set_level(RAMP_SIZE);
            _delay_s();
            set_level(0);
            _delay_s();
        }
#endif // DELAY_TEST
#ifdef TEMP_TEST
        else if (output == TEMP_TEST) {
            debug_out(get_temp(), 3);
        }
#endif // TEMP_TEST
#ifdef VOLTAGE_TEST
        else if (output == VOLTAGE_TEST) {
            debug_out(get_voltage(), 3);
        }
#endif // VOLTAGE_TEST
#ifdef OFFTIM_TEST
        else if (output == OFFTIM_TEST) {
            debug_out(cap_val, 5);
        }
#endif // OFFTIM_TEST
        else {  // Regular non-hidden solid mode
            set_mode(actual_level);
#ifdef TEMPERATURE_MON
            uint8_t temp = get_temp();

            // step down? (or step back up?)
            if (temp >= maxtemp) {
                overheat_count ++;
                // reduce noise, and limit the lowest step-down level
                if ((overheat_count > 15) && (actual_level > (RAMP_SIZE/8))) {
                    actual_level --;
                    //_delay_ms(5000);  // don't ramp down too fast
                    overheat_count = 0;  // don't ramp down too fast
                }
            } else {
                // if we're not overheated, ramp up to the user-requested level
                overheat_count = 0;
                if ((temp < maxtemp - 2) && (actual_level < output)) {
                    actual_level ++;
                }
            }
            set_mode(actual_level);

            ADC_on();  // return to voltage mode
#endif
            // Otherwise, just sleep.
            _delay_ms(500);

            // If we got this far, the user has stopped fast-pressing.
            // So, don't enter config mode.
            //reset_fast_presses();
        }
        reset_fast_presses();
#ifdef VOLTAGE_MON
        if (ADM & (1 << 5)) {  // if a voltage reading is ready
            ADM &= ~(1 << 5); // Clear EOC bit
            voltage = (255-ADB);  // get the waiting value (8-bit)
            // See if voltage is lower than what we were looking for
            if (voltage < ADC_LOW) {
                lowbatt_cnt ++;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= 8) {
                // DEBUG: blink on step-down:
                //set_level(0);  _delay_ms(100);

                if (actual_level > RAMP_SIZE) {  // hidden / blinky modes
                    // step down from blinky modes to medium
                    actual_level = RAMP_SIZE / 2;
                } else if (actual_level > 1) {  // regular solid mode
                    // step down from solid modes somewhat gradually
                    // drop by 25% each time
                    actual_level = (actual_level >> 2) + (actual_level >> 1);
                    // drop by 50% each time
                    //actual_level = (actual_level >> 1);
                } else { // Already at the lowest mode
                    //mode_idx = 0;  // unnecessary; we never leave this clause
                    //actual_level = 0;  // unnecessary; we never leave this clause
                    // Turn off the light
                    set_level(0);
                    // Power down as many components as possible
                    STOP();
                }
                set_mode(actual_level);
                output = actual_level;
                //save_mode();  // we didn't actually change the mode
                lowbatt_cnt = 0;
                // Wait before lowering the level again
                //_delay_ms(250);
                _delay_s();
            }

            // Make sure conversion is running for next time through
            ADM |= (1 << 6);
        }
#endif  // ifdef VOLTAGE_MON
    }

    //return 0; // Standard Return Code
}
