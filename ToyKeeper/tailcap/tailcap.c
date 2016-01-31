/*
 * Drive a smart lighted tailcap with RGB LEDs.
 *
 * On each boot, it will do a colorful twirl for a bit, read the cell voltage,
 * then stop the swirl on a specific color depending on voltage.  After a
 * while, it will then dim or turn off to save power, blinking every few
 * seconds to act as a locator and voltage indicator.
 *
 * NANJG 105C Diagram
 *           ----
 *         -|1  8|- VCC
 *  unused -|2  7|- Voltage ADC
 *    blue -|3  6|- green
 *     GND -|4  5|- red
 *           ----
 *
 * CPU speed is 4.8Mhz without the 8x divider when low fuse is 0x75
 *
 * FUSES
 *      I use these fuse settings
 *      Low:  0x75
 *      High: 0xff
 *
 */
// set some hardware-specific values...
#define NANJG_LAYOUT  // specify an I/O pin layout
// Also, assign I/O pins in this file:
#include "tk-attiny.h"
// Configure actual pins per color here
#define RED_PIN PB4
#define GRN_PIN PB0
#define BLU_PIN PB1

// How many milliseconds to spin at boot before measuring
#define SPIN_UP_TIME 1000
// milliseconds to pause at each color while spinning
#define SPIN_SPEED 33
// keep tail light on while resting
//#define ALWAYS_ON
// re-spin in standby mode
#define ALWAYS_SPIN_UP
// spin at least one full circle each time in standby mode
//#define ALWAYS_FULL_SPIN

// pick only one of the following
//#define BEACON_2s
#define BEACON_4s
//#define BEACON_8s

// choose how long to leave the light on for each beacon flash
// (only enable one of the following lines)
//#define BEACON_BLINK _delay_ms(100)
//#define BEACON_BLINK _delay_ms(250)
//#define BEACON_BLINK _delay_ms(500)
#define BEACON_BLINK _delay_s()

/*
 * =========================================================================
 */

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_S         // Also use _delay_s(), not just _delay_ms()
#include "tk-delay.h"

#define VOLTAGE_MON
#define USE_BATTCHECK
#define BATTCHECK_VpT  // Volts + tenths

// Calibrate voltage and OTC in this file:
#include "tk-calibration.h"

#include "tk-voltage.h"

// voltage levels at which to change color
uint8_t voltage_steps[] = {
    0,         // red
    ADC_33,    // yellow above 3.3V
    ADC_36,    // green above 3.6V
    ADC_38,    // cyan above 3.8V
    ADC_40,    // blue above 4.0V
    255,       // Ceiling, don't remove
};

// color sequence from lowest to highest voltage
uint8_t spin_steps[] = {
    (1<<RED_PIN),
    (1<<GRN_PIN) | (1<<RED_PIN),
    (1<<GRN_PIN),
    (1<<BLU_PIN) | (1<<GRN_PIN),
    (1<<BLU_PIN),
    (1<<RED_PIN) | (1<<BLU_PIN),
    (1<<RED_PIN) | (1<<BLU_PIN) | (1<<GRN_PIN),
};


void go_white() {
    PORTB = (1<<RED_PIN) | (1<<GRN_PIN) | (1<<BLU_PIN);
    DDRB = (1<<RED_PIN) | (1<<GRN_PIN) | (1<<BLU_PIN);
}

void go_dark() {
    PORTB = 0;
    DDRB = (1<<RED_PIN) | (1<<GRN_PIN) | (1<<BLU_PIN);
    //DDRB = 0;
}

void blink(uint8_t num) {
    for(; num>0; num--) {
        go_white();
        _delay_ms(100);
        go_dark();
        _delay_ms(200);
    }
}

void spin(uint8_t steps, uint16_t speed) {
    uint8_t i;
    for(i=0; i<steps; i++) {
        PORTB = spin_steps[i];
        //DDRB = spin_steps[i];
        DDRB = (1<<RED_PIN) | (1<<GRN_PIN) | (1<<BLU_PIN);
        _delay_ms(speed);
    }
}

inline uint8_t pick_step() {
    // Return an int, number of "blinks", for approximate battery charge
    // Uses the table above for return values
    uint8_t i, voltage;
    voltage = get_voltage();
    // figure out how many times to blink
    for (i=0;
         voltage > voltage_steps[i];
         i ++) {}
    return i;
}

void spin_to_voltage() {
    uint8_t i = pick_step();
    //blink(i); _delay_s();
    spin(i, SPIN_SPEED*2);
    //spin(i, SPIN_SPEED*8);
}

void direct_to_voltage() {
    uint8_t i = pick_step();
    spin(i, 0);
}

inline void WDT_on() {
    // Setup watchdog timer to only interrupt, not reset, every 16ms.
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
#ifdef BEACON_2s
    // Enable interrupt every 2.0 s
    WDTCR = (1<<WDTIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
#endif
#ifdef BEACON_4s
    // Enable interrupt every 4.0 s
    WDTCR = (1<<WDTIE) | (1<<WDP3);
#endif
#ifdef BEACON_8s
    // Enable interrupt every 8.0 s
    WDTCR = (1<<WDTIE) | (1<<WDP3) | (1<<WDP0);
#endif
    sei();                          // Enable interrupts
}

ISR(WDT_vect) {
    // No need to do anything here...
    // ... just wake up
}

void go_to_sleep() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
}

int main(void)
{
    // Set pins to output
    DDRB = (1<<RED_PIN) | (1<<GRN_PIN) | (1<<BLU_PIN);

    // Disable PWM  (FIXME: is this needed?)
    TCCR0A = 0; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Turn features on or off as needed
    ADC_on();
    ACSR   |=  (1<<7); //AC off  (FIXME: not sure what this does)

    get_voltage();  // first one doesn't count, prime for later

    uint8_t i;

    // spin for a second or so
    for(i=0; i<(SPIN_UP_TIME/SPIN_SPEED/6); i++) {
        spin(6, SPIN_SPEED);
    }

    spin_to_voltage();
    _delay_s();
    _delay_s();
    _delay_s();

    WDT_on();

    while(1) {
#ifndef ALWAYS_ON
        // turn off tail light while waiting
        go_dark();
#endif  // ALWAYS_ON

        // low power mode until WDT wakes us up
        go_to_sleep();

#ifdef ALWAYS_FULL_SPIN
        spin(6, SPIN_SPEED);
#endif
#ifdef ALWAYS_SPIN_UP
        spin_to_voltage();
#else
        direct_to_voltage();
#endif

        // keep the light on for a short while
        // (blink like a beacon)
        BEACON_BLINK;
    }
}
