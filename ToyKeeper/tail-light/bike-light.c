/* Bike tail light firmware
 * Copyright (C) 2026 Selene ToyKeeper
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Usage:
 * - Long press to reset to first mode.
 * - Short press within a few seconds to advance to next mode.
 * - After a few seconds, mode is locked; can't be changed by short press.
 *   This is to ensure the light won't change modes when going over bumps,
 *   which tend to disconnect power briefly.
 * - Modes start with the ones most useful for biking.
 *
 * This is intended for use on flashlights with a clicky switch,
 * and a single channel nanjg attiny13 driver.
 *
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *         -|   |- Voltage ADC
 *         -|   |- PWM
 *     GND -|   |-
 *           ---
 *
 * FUSES
 *      (check bin/flash*.sh for recommended values)
 *
 * VOLTAGE
 *      Resistor values for voltage divider (reference BLF-VLD README for more info)
 *      Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *           VCC
 *            |
 *           Vd (~.25 v drop from protection diode)
 *            |
 *          1912 (R1 19,100 ohms)
 *            |
 *            |---- PB2 from MCU
 *            |
 *          4701 (R2 4,700 ohms)
 *            |
 *           GND
 *
 *      ADC = ((V_bat - V_diode) * R2   * 255) / ((R1    + R2  ) * V_ref)
 *      125 = ((3.0   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *      121 = ((2.9   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *
 *      Well 125 and 121 were too close, so it shut off right after lowering to low mode, so I went with
 *      130 and 120
 *
 *      To find out what value to use, plug in the target voltage (V) to this equation
 *          value = (V * 4700 * 255) / (23800 * 1.1)
 *
 */
#define NANJG_LAYOUT
#include "tk-attiny.h"
//#undef BOGOMIPS  // adjust speed for individual drivers
//#define BOGOMIPS 890

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON                 // Comment out to disable
#define OWN_DELAY                   // Should we use the built-in delay or our own?
#define USE_DELAY_MS
#define USE_FINE_DELAY

#define FAST_PWM_START      10      // Anything under this will use phase-correct

////////// mode array values //////////
// We're committing a sin here.  Different array cells have different data
// types, all mixed together... and it's important to make sure none of the
// values collide.  This is a dirty hack which is done to save ROM space.
// (uses an enum to ensure there are no value collisions)
enum MODE_VALUES {
    // solid modes: value = PWM level
    MODE_MOON           = 4,
    MODE_LOW            = 14,
    MODE_MED            = 39,
    MODE_HIGH           = 120,
    MODE_MAX            = 255,
    // battery check mode
    MODE_BATTCHECK      = 254,  // unique ID
    // heartbeat style beacon
    MODE_HEART_BEACON   = 253,  // unique ID
    // bike flashers: value = 0/1/2 for which solid mode to use as a base
    MODE_BIKE_LOW       = 0,  // moon+med
    MODE_BIKE_MED       = 1,  // low+high
    MODE_BIKE_HIGH      = 2,  // med+max
    // constant-speed strobe modes: value = delay in ms between pulses
    MODE_STROBE_SLOW    = 82,  // 12 Hz
    MODE_STROBE_MED     = 41,  // 24 Hz
    MODE_STROBE_FAST    = 15,  // 60 Hz
    // variable-speed strobe modes
    MODE_STROBE_VSLOW   = 252,  // unique ID
    MODE_STROBE_VFAST   = 251,  // unique ID
};

#define USE_BATTCHECK
#define BATTCHECK_VpT  // Use the volts+tenths battcheck style
//#define BATTCHECK_4bars  // Use the volts+tenths battcheck style

// number of WDT ticks before mode is saved (.5 sec each)
#define WDT_TIMEOUT       6  // 6 ticks = 3 seconds

#include "tk-calibration.h"

/*
 * =========================================================================
 */

#include "tk-delay.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "tk-voltage.h"

/*
 * global variables
 */

// Mode storage
// store in uninitialized memory so it will not be overwritten and
// can still be read at startup after short (<500ms) power off
// decay used to tell if user did a short press.
// detect short/long press
volatile uint8_t noinit_decay __attribute__ ((section (".noinit")));
// which mode the user is in
volatile uint8_t noinit_mode __attribute__ ((section (".noinit")));
// whether mode is locked
volatile uint8_t noinit_locked __attribute__ ((section (".noinit")));

// Modes (hardcoded to save space)
#define MODE_SOLID  5  // index of first solid mode
const uint8_t modes[] = {
    // start on modes most useful for biking
    MODE_BATTCHECK,
    MODE_HEART_BEACON,
    MODE_BIKE_LOW,
    MODE_BIKE_MED,
    MODE_BIKE_HIGH,
    // regular solid modes
    MODE_MOON,  // <-- point MODE_SOLID at this array index
    MODE_LOW,
    MODE_MED,
    MODE_HIGH,
    MODE_MAX,
    // other blinkies, not used as much
    MODE_STROBE_SLOW,
    MODE_STROBE_MED,
    MODE_STROBE_FAST,
    MODE_STROBE_VSLOW,
    MODE_STROBE_VFAST,
};
volatile uint8_t mode_idx = 0;

inline void next_mode() {
    mode_idx += 1;
    // wrap around
    if (mode_idx >= (sizeof(modes))) { mode_idx = 0; }
}

#define BLINK_BRIGHTNESS MODE_MED
#define BLINK_SPEED 500
#ifdef BATTCHECK_VpT
void blink(uint8_t val, uint16_t speed)
{
    for (; val>0; val--)
    {
        PWM_LVL = BLINK_BRIGHTNESS;
        _delay_ms(speed);
        PWM_LVL = 0;
        _delay_ms(speed<<2);
    }
}
#else
void blink(uint8_t val)
{
    for (; val>0; val--)
    {
        PWM_LVL = BLINK_BRIGHTNESS;
        _delay_ms(100);
        PWM_LVL = 0;
        _delay_ms(400);
    }
}
#endif

inline void WDT_on() {
    // Setup watchdog timer to only interrupt, not reset, every 500ms.
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = (1<<WDTIE) | (1<<WDP2) | (1<<WDP0); // Enable interrupt every 500ms
    sei();                          // Enable interrupts
}

inline void WDT_off()
{
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    MCUSR &= ~(1<<WDRF);            // Clear Watchdog reset flag
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = 0x00;                   // Disable WDT
    sei();                          // Enable interrupts
}

ISR(WDT_vect) {
    static uint8_t ticks = 0;
    if (ticks < 255) ticks++;

    if (ticks >= WDT_TIMEOUT) {
        noinit_locked = 1;  // lock in current mode
        WDT_off();  // no need for this any more
    }
}

int main(void)
{
    // All ports default to input, but turn pull-up resistors on for the stars
    // (not the ADC input!  Made that mistake already)
    // (stars not used)
    //PORTB = (1 << STAR2_PIN) | (1 << STAR3_PIN) | (1 << STAR4_PIN);

    // Set PWM pin to output
    DDRB = (1 << PWM_PIN);

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif
    ACSR   |=  (1<<7); //AC off

    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts (which we're not using anyway)
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Determine what mode we should fire up
    // Read the last mode that was saved
    if (noinit_decay)  // not short press, forget mode
    {
        noinit_mode = 0;
        mode_idx = 0;
        noinit_locked = 0;  // allow changing modes
    } else {  // short press
        // advance to next mode if not locked
        mode_idx = noinit_mode;
        if (! noinit_locked) { next_mode(); }
        noinit_mode = mode_idx;
    }
    // set noinit data for next boot
    noinit_decay = 0;

    // set PWM mode
    if (modes[mode_idx] < FAST_PWM_START) {
        // Set timer to do PWM for correct output pin and set prescaler timing
        TCCR0A = 0x21; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    } else {
        // Set timer to do PWM for correct output pin and set prescaler timing
        TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    }
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    WDT_on();

    // Now just fire up the mode
    //PWM_LVL = modes[mode_idx];

    //uint8_t i = 0;
    //uint8_t j = 0;
    //uint8_t strobe_len = 0;
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t voltage;
#endif
    while(1) {
        uint8_t mode = modes[mode_idx];
        switch (mode) {

            case MODE_BATTCHECK:
                PWM_LVL = 0;
                get_voltage();  _delay_ms(200);  // the first reading is junk
                #ifdef BATTCHECK_VpT
                    uint8_t result = battcheck();
                    blink(result >> 5, BLINK_SPEED/8);
                    _delay_ms(BLINK_SPEED);
                    blink(1,5);
                    _delay_ms(BLINK_SPEED*3/2);
                    blink(result & 0b00011111, BLINK_SPEED/8);
                #else
                    blink(battcheck());
                #endif  // BATTCHECK_VpT
                _delay_ms(2000);  // wait at least 2 seconds between readouts
                break;

            case MODE_HEART_BEACON:  // heartbeat flasher
                PWM_LVL = MODE_MAX;  _delay_ms(1);
                PWM_LVL = 0;         _delay_ms(249);
                PWM_LVL = MODE_MAX;  _delay_ms(1);
                PWM_LVL = 0;         _delay_ms(749);
                break;

            case MODE_STROBE_VSLOW:
                // strobe mode, smoothly oscillating frequency ~7 Hz to ~18 Hz
                for(uint8_t j=0; j<66; j++) {
                    PWM_LVL = MODE_MAX;
                    _delay_ms(1);
                    PWM_LVL = 0;
                    uint8_t strobe_len;
                    if (j < 33) { strobe_len = j; }
                    else { strobe_len = 66-j; }
                    _delay_ms(2 * (strobe_len+33-6));
                }
                break;

            case MODE_STROBE_VFAST:
                // strobe mode, smoothly oscillating frequency ~16 Hz to ~100 Hz
                for(uint8_t j=0; j<100; j++) {
                    PWM_LVL = MODE_MAX;
                    _delay_zero(); // less than a millisecond
                    PWM_LVL = 0;
                    uint8_t strobe_len;
                    if (j < 50) { strobe_len = j; }
                    else { strobe_len = 100-j; }
                    _delay_ms(strobe_len+9);
                }
                break;

            case MODE_STROBE_SLOW:
            case MODE_STROBE_MED:
            case MODE_STROBE_FAST:
                // strobe mode, fixed-speed
                PWM_LVL = MODE_MAX;
                if (mode < 50) { _delay_zero(); }
                else { _delay_ms(1); }
                PWM_LVL = 0;
                _delay_ms(mode);
                break;

            case MODE_BIKE_LOW:
            case MODE_BIKE_MED:
            case MODE_BIKE_HIGH:
                // two-level fast strobe pulse at about 1 Hz
                uint8_t low  = modes[MODE_SOLID + mode];
                uint8_t high = modes[MODE_SOLID + mode + 2];
                for(uint8_t i=0; i<4; i++) {
                    PWM_LVL = high;  _delay_ms(5);
                    PWM_LVL = low;   _delay_ms(65);
                }
                _delay_ms(720);
                break;

            default:  // regular solid modes
                // just stay on at a given brightness
                PWM_LVL = mode;
                sleep_mode();
                break;

        }
#ifdef VOLTAGE_MON
        if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
            voltage = get_voltage();
            // See if voltage is lower than what we were looking for
            if (voltage < ((mode_idx == 0) ? ADC_CRIT : ADC_LOW)) {
                ++lowbatt_cnt;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= 3) {
                if (mode_idx > 0) {
                    mode_idx = 0;
                } else { // Already at the lowest mode
                    // Turn off the light
                    PWM_LVL = 0;
                    // Disable WDT so it doesn't wake us up
                    WDT_off();
                    // Power down as many components as possible
                    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                    sleep_mode();
                }
                lowbatt_cnt = 0;
                // Wait at least 1 second before lowering the level again
                _delay_ms(1000);  // this will interrupt blinky modes
            }

            // Make sure conversion is running for next time through
            ADCSRA |= (1 << ADSC);
        }
#endif
    }
}
