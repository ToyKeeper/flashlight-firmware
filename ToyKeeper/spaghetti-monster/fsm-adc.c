/*
 * fsm-adc.c: ADC (voltage, temperature) functions for SpaghettiMonster.
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

#ifndef FSM_ADC_C
#define FSM_ADC_C

#include <stdlib.h>

static inline void set_admux_therm() {
    #if (ATTINY == 25) || (ATTINY == 45) || (ATTINY == 85) || (ATTINY == 1634)
        ADMUX = ADMUX_THERM;
    #elif (ATTINY == 841)
        ADMUXA = ADMUXA_THERM;
        ADMUXB = ADMUXB_THERM;
    #else
        #error Unrecognized MCU type
    #endif
}

inline void set_admux_voltage() {
    #if (ATTINY == 25) || (ATTINY == 45) || (ATTINY == 85) || (ATTINY == 1634)
        #ifdef USE_VOLTAGE_DIVIDER
        // 1.1V / pin7
        ADMUX = ADMUX_VOLTAGE_DIVIDER;
        #else
        // VCC / 1.1V reference
        ADMUX = ADMUX_VCC;
        #endif
    #elif (ATTINY == 841)
        #ifdef USE_VOLTAGE_DIVIDER
        ADMUXA = ADMUXA_VOLTAGE_DIVIDER;
        ADMUXB = ADMUXB_VOLTAGE_DIVIDER;
        #else
        ADMUXA = ADMUXA_VCC;
        ADMUXB = ADMUXB_VCC;
        #endif
    #else
        #error Unrecognized MCU type
    #endif
}

inline void ADC_start_measurement() {
    #if (ATTINY == 25) || (ATTINY == 45) || (ATTINY == 85) || (ATTINY == 841) || (ATTINY == 1634)
        ADCSRA |= (1 << ADSC) | (1 << ADIE);
    #else
        #error unrecognized MCU type
    #endif
}

inline void ADC_stop_measurement() {
    #if (ATTINY == 25) || (ATTINY == 45) || (ATTINY == 85) || (ATTINY == 841) || (ATTINY == 1634)
        ADCSRA &= ~(1 << ADIF | 1 << ADIE);
    #else
        #error unrecognized MCU type
    #endif
}

// set up ADC for reading battery voltage
inline void ADC_on()
{
    #if (ATTINY == 25) || (ATTINY == 45) || (ATTINY == 85) || (ATTINY == 1634)
        set_admux_voltage();
        #ifdef USE_VOLTAGE_DIVIDER
        // disable digital input on divider pin to reduce power consumption
        DIDR0 |= (1 << VOLTAGE_ADC_DIDR);
        #else
        // disable digital input on VCC pin to reduce power consumption
        //DIDR0 |= (1 << ADC_DIDR);  // FIXME: unsure how to handle for VCC pin
        #endif
        #if (ATTINY == 1634)
            ACSRA |= (1 << ACD);  // turn off analog comparator to save power
        #endif
        // enable, start, prescale
        ADCSRA = (1 << ADEN) | (1 << ADSC) | ADC_PRSCL;
        // end tiny25/45/85
    #elif (ATTINY == 841)
        ADCSRB = 0;  // Right adjusted, auto trigger bits cleared.
        //ADCSRA = (1 << ADEN ) | 0b011;  // ADC on, prescaler division factor 8.
        set_admux_voltage();
        // enable, start, prescale
        ADCSRA = (1 << ADEN) | (1 << ADSC) | ADC_PRSCL;
        //ADCSRA |= (1 << ADSC);  // start measuring
    #else
        #error Unrecognized MCU type
    #endif
}

inline void ADC_off() {
    ADCSRA &= ~(1<<ADEN); //ADC off
}

// happens every time the ADC sampler finishes a measurement
ISR(ADC_vect) {
    #define ADC_PLUS_BITS 8 // oversampling strength
    static uint8_t  adc_stable = 0;
    static uint16_t adc_sum_counter = 0;
    static uint32_t adc_sum = 0;

    ADC_start_measurement(); // mandatory in standby mode

    if (!adc_stable) {
        adc_stable = 1;
        #ifdef USE_PSEUDO_RAND
        // real-world entropy makes this a true random, not pseudo
        pseudo_rand_seed += ADCL;
        #endif
        return;
    }

    if (adc_sum_counter < 10 * (1 << ADC_PLUS_BITS)) {
        adc_sum += ADC; // latest 10-bit ADC reading
        adc_sum_counter++;
        if (!go_to_standby) return;
    }
    adc_10x = go_to_standby ? ADC * 10 : adc_sum >> ADC_PLUS_BITS;
    adc_sum_counter = adc_sum = 0;

    adc_stable = 0; // set the dirty flag for the next sequence
    irq_adc = 1; // enough samples accumulated, trigger deferred logic

    ADC_stop_measurement(); // abort free running mode
}

void ADC_inner() {
    irq_adc = 0;  // event handled

    // what is being measured? 0 = battery voltage, 1 = temperature
    static uint8_t adc_type = 0;

    if (!adc_type) {  // voltage
        #if defined(USE_LVP) || defined(USE_SLEEP_LVP) || defined(USE_BATTCHECK)
            ADC_voltage_handler();
        #endif
        #ifdef USE_THERMAL_REGULATION
            if (!go_to_standby) {  // only measure battery voltage while asleep
                set_admux_therm();
                adc_type = 1;
            }
        #endif
    } else {  // temperature
        #ifdef USE_THERMAL_REGULATION
            ADC_temperature_handler();
            #if defined(USE_LVP) || defined(USE_SLEEP_LVP) || defined(USE_BATTCHECK)
            set_admux_voltage();
            #endif
            adc_type = 0; // always skip one cycle, even if we don't measure battery voltage
        #endif
    }

    #ifdef TICK_DURING_STANDBY
        // in sleep mode, turn off after just one measurement
        // (having the ADC on raises standby power by about 250 uA)
        // (and the usual standby level is only ~20 uA)
        if (go_to_standby) ADC_off();
    #endif
}

#if defined(USE_LVP) || defined(USE_SLEEP_LVP) || defined(USE_BATTCHECK)
// Runs once per second (unless in standby)
static inline void ADC_voltage_handler() {
    uint16_t measurement = (adc_10x + 5) / 10;
    #ifdef USE_VOLTAGE_DIVIDER
    // use 9.7 fixed-point to get sufficient precision
    uint16_t adc_per_volt = ((ADC_44<<7) - (ADC_22<<7)) / (44-22);
    // incoming value is 8.2 fixed-point, so shift it 2 bits less
    voltage = ((measurement<<5) / adc_per_volt) + VOLTAGE_FUDGE_FACTOR;
    #else
    // calculate actual voltage: volts * 10
    // ADC = 1.1 * 1024 / volts
    // volts = 1.1 * 1024 / ADC
    //voltage = (uint16_t)(1.1*1024*10)/measurement + VOLTAGE_FUDGE_FACTOR;
    voltage = ((uint16_t)(2*1.1*1024*10)/measurement + VOLTAGE_FUDGE_FACTOR) >> 1;
    #endif
    #if defined(USE_LVP) || defined(USE_SLEEP_LVP)
    if (voltage < VOLTAGE_LOW) {
        emit(EV_voltage_low, 0);
    }
    #endif  // ifdef USE_LVP
}
#endif


#ifdef USE_THERMAL_REGULATION
// Runs once per second (unless in standby)
static inline void ADC_temperature_handler() {
    #define NUM_HISTORY_TEMPS 4 // hardcoded, not intended to be changed
    #ifndef THERM_LOOKAHEAD
    #define THERM_LOOKAHEAD 5 // might be lowered with lower power to mass ratios
    #endif
    const  int16_t temp_target = therm_ceil * 10 - 5;
    static int16_t temp_history[NUM_HISTORY_TEMPS];
    static uint8_t temp_history_index = 0;
    static uint8_t temp_record_offset = 0;
    static int16_t temp_offset_100x = 0;

    int16_t temp_10x = adc_10x + 10 * (THERM_CAL_OFFSET + therm_cal_offset - 275);
    temperature = (temp_10x + 5) / 10; // save temperature C (ish) for later use in the ui

    // prime on first execution
    if (reset_thermal_history) {
        reset_thermal_history = 0;
        temp_record_offset = 0;
        temp_offset_100x = 0;
        for (uint8_t i=0; i<NUM_HISTORY_TEMPS; i++)
            temp_history[i] = temp_10x;
    }

    int16_t temp_slope = temp_10x - temp_history[temp_history_index];
    temp_history[temp_history_index] = temp_10x;
    temp_history_index = (temp_history_index + 1) % NUM_HISTORY_TEMPS;

    int16_t predicted_temp = temp_10x + temp_slope * THERM_LOOKAHEAD;
    int16_t offset = predicted_temp - temp_target + temp_offset_100x / 10;

    if (offset > 5 && temp_slope >= 0) {
        temp_record_offset = 5;
        int16_t howmuch = (offset / 6) + (temp_slope >> 1);
        emit(EV_temperature_high, howmuch);
    } else if (temp_record_offset) {
        temp_record_offset--;
        int16_t to_100x = 4 * (temp_target - temp_10x);
        if (!temp_record_offset && to_100x > temp_offset_100x) {
            temp_offset_100x = to_100x;
        }
    } else if ((offset < -15 && temp_slope < -15) || (temp_slope <= 0 && offset < -50)) {
        #ifdef USE_LVP
        if (voltage > VOLTAGE_LOW)
        #endif
        {
            int16_t howmuch = (-offset / 12) + (-temp_slope >> 1);
            emit(EV_temperature_low, howmuch);
        }
    } else if (temp_offset_100x > 0) {
        temp_offset_100x--;
    }
}
#endif  // ifdef USE_THERMAL_REGULATION


#ifdef USE_BATTCHECK
#ifdef BATTCHECK_4bars
PROGMEM const uint8_t voltage_blinks[] = {
    30, 35, 38, 40, 42, 99,
};
#endif
#ifdef BATTCHECK_6bars
PROGMEM const uint8_t voltage_blinks[] = {
    30, 34, 36, 38, 40, 41, 43, 99,
};
#endif
#ifdef BATTCHECK_8bars
PROGMEM const uint8_t voltage_blinks[] = {
    30, 33, 35, 37, 38, 39, 40, 41, 42, 99,
};
#endif
void battcheck() {
    #ifdef BATTCHECK_VpT
    blink_num(voltage);
    #else
    uint8_t i;
    for(i=0;
        voltage >= pgm_read_byte(voltage_blinks + i);
        i++) {}
    #ifdef DONT_DELAY_AFTER_BATTCHECK
    blink_digit(i);
    #else
    if (blink_digit(i))
        nice_delay_ms(1000);
    #endif
    #endif
}
#endif

#endif
