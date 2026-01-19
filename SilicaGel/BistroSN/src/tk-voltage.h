// tk-voltage.h: Voltage / battcheck functions.
// Copyright (C) 2015-2023 Selene ToyKeeper (original ATtiny version)
// Copyright (C) 2025 SilicaGel (SN8F5xxx port)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "tk-sn8f5xxx.h"
#include "tk-calibration.h"

#if defined(TEMPERATURE_MON) || defined(THERMAL_REGULATION)
#ifdef TEMP_12bit
#define NEED_ADC_12bit
// Flip ADC reading to avoid changing logic elsewere
#define get_temperature() (4095-read_adc_12bit())
#else
// Flip ADC reading to avoid changing logic elsewere
#define get_temperature() (255-read_adc_8bit())
#endif

inline void ADC_on_temperature(void) {
    ADM = (uint8_t)(1 << 7) | (uint8_t)(TEMP_CHANNEL << 0); // enable ADC and set channel
    ADR = (1 << 6) | (2 << 4); // enable ADC channel and set speed to fosc/1
    VREFH = 4; // VREFH = VDD, AIN8 = 2.0V
}
#endif  // TEMPERATURE_MON

#ifdef VOLTAGE_MON
#define NEED_ADC_8bit
inline void ADC_on(void) {
    ADM = (uint8_t)(1 << 7) | (uint8_t)(VOLTAGE_CHANNEL << 0); // enable ADC and set channel
    ADR = (1 << 6) | (2 << 4); // enable ADC channel and set speed to fosc/1
    VREFH = 4; // VREFH = VDD, AIN8 = 2.0V
}

// Flip ADC reading to avoid changing logic elsewere
// TODO: refactor!!!
#define get_voltage() (255-read_adc_8bit())
#else
inline void ADC_off(void) {
    ADM &= ~(1 << 7); // disable ADC
}
#endif

#ifdef NEED_ADC_8bit
uint8_t read_adc_8bit(void) {
    ADM &= ~(1 << 5); // Clear EOC bit
    ADM |= (1 << 6); // start ADC conversion
    while (!(ADM & (1 << 5))) {} // Wait for conversion
    return ADB; // Return 8-bit result
}
#endif

#ifdef NEED_ADC_12bit
uint16_t read_adc_12bit(void) {
    ADM &= ~(1 << 5); // Clear EOC bit
    ADM |= (1 << 6); // start ADC conversion
    while (!(ADM & (1 << 5))) {} // Wait for conversion
    return ((uint16_t)ADB) << 4 | (ADR & 0x0F); // Return 12-bit result
}
#endif

#ifdef USE_BATTCHECK
#ifdef BATTCHECK_4bars
__code const uint8_t voltage_blinks[] = {
               // 0 blinks for less than 1%
    ADC_0p,    // 1 blink  for 1%-25%
    ADC_25p,   // 2 blinks for 25%-50%
    ADC_50p,   // 3 blinks for 50%-75%
    ADC_75p,   // 4 blinks for 75%-100%
    ADC_100p,  // 5 blinks for >100%
    255,       // Ceiling, don't remove  (6 blinks means "error")
};
#endif  // BATTCHECK_4bars
#ifdef BATTCHECK_8bars
__code const uint8_t voltage_blinks[] = {
               // 0 blinks for less than 1%
    ADC_30,    // 1 blink  for 1%-12.5%
    ADC_33,    // 2 blinks for 12.5%-25%
    ADC_35,    // 3 blinks for 25%-37.5%
    ADC_37,    // 4 blinks for 37.5%-50%
    ADC_38,    // 5 blinks for 50%-62.5%
    ADC_39,    // 6 blinks for 62.5%-75%
    ADC_40,    // 7 blinks for 75%-87.5%
    ADC_41,    // 8 blinks for 87.5%-100%
    ADC_42,    // 9 blinks for >100%
    255,       // Ceiling, don't remove  (10 blinks means "error")
};
#endif  // BATTCHECK_8bars
#ifdef BATTCHECK_VpT
/*
__code const uint8_t v_whole_blinks[] = {
               // 0 blinks for (shouldn't happen)
    0,         // 1 blink for (shouldn't happen)
    ADC_20,    // 2 blinks for 2V
    ADC_30,    // 3 blinks for 3V
    ADC_40,    // 4 blinks for 4V
    255,       // Ceiling, don't remove
};
__code const uint8_t v_tenth_blinks[] = {
               // 0 blinks for less than 1%
    ADC_30,
    ADC_33,
    ADC_35,
    ADC_37,
    ADC_38,
    ADC_39,
    ADC_40,
    ADC_41,
    ADC_42,
    255,       // Ceiling, don't remove
};
*/
__code const uint8_t voltage_blinks[] = {
    // 0 blinks for (shouldn't happen)
    ADC_25,(2<<5)+5,
    ADC_26,(2<<5)+6,
    ADC_27,(2<<5)+7,
    ADC_28,(2<<5)+8,
    ADC_29,(2<<5)+9,
    ADC_30,(3<<5)+0,
    ADC_31,(3<<5)+1,
    ADC_32,(3<<5)+2,
    ADC_33,(3<<5)+3,
    ADC_34,(3<<5)+4,
    ADC_35,(3<<5)+5,
    ADC_36,(3<<5)+6,
    ADC_37,(3<<5)+7,
    ADC_38,(3<<5)+8,
    ADC_39,(3<<5)+9,
    ADC_40,(4u<<5)+0u,
    ADC_41,(4u<<5)+1u,
    ADC_42,(4u<<5)+2u,
    ADC_43,(4u<<5)+3u,
    ADC_44,(4u<<5)+4u,
    255,   (1u<<5)+1u,  // Ceiling, don't remove
};
inline uint8_t battcheck(void) {
    // Return an composite int, number of "blinks", for approximate battery charge
    // Uses the table above for return values
    // Return value is 3 bits of whole volts and 5 bits of tenths-of-a-volt
    uint8_t i, voltage;
    voltage = get_voltage();
    // figure out how many times to blink
    for (i=0;
         voltage > voltage_blinks[i];
         i += 2) {}
    return voltage_blinks[i + 1];
}
#else  // #ifdef BATTCHECK_VpT
inline uint8_t battcheck(void) {
    // Return an int, number of "blinks", for approximate battery charge
    // Uses the table above for return values
    uint8_t i, voltage;
    voltage = get_voltage();
    // figure out how many times to blink
    for (i=0;
         voltage > voltage_blinks[i];
         i ++) {}
    return i;
}
#endif  // BATTCHECK_VpT
#endif

