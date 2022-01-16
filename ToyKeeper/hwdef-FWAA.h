#ifndef HWDEF_FWAA_H
#define HWDEF_FWAA_H

/* BLF/TLF FWAA driver layout
 *              ----
 * Reset (NC) -|1  8|- VCC
 *    eswitch -|2  7|- N/C
 *    Voltage -|3  6|- FET
 *        GND -|4  5|- 1x7135
 *              ----
 */

#define PWM_CHANNELS 2

#ifndef SWITCH_PIN
#define SWITCH_PIN   PB3    // pin 2
#define SWITCH_PCINT PCINT3 // pin 2 pin change interrupt
#endif

#ifndef PWM1_PIN
#define PWM1_PIN PB0        // pin 5, 1x7135 PWM
#define PWM1_LVL OCR0A      // OCR0A is the output compare register for PB0
#endif
#ifndef PWM2_PIN
#define PWM2_PIN PB1        // pin 6, FET PWM
#define PWM2_LVL OCR0B      // OCR0B is the output compare register for PB1
#endif


#define USE_VOLTAGE_DIVIDER
#define VOLTAGE_ADC ADC2D
#define VOLTAGE_PIN PB4
#define VOLTAGE_ADC_DIDR DIDR0
#define VOLTAGE_CHANNEL 0b10 // MUX 10 = ADC2 (PB4)
// datasheet 17.13.1
// REFS1 REFS0 ADLAR REFS2 MUX3 MUX2 MUX1 MUX0
// REFS for 1.1V internal: REFS0=0, REFS1=1, REFS2=0
// MUX for PB4: 0010
//#define ADMUX_VOLTAGE_DIVIDER 0b10000010
#define ADMUX_VOLTAGE_DIVIDER ((1 << V_REF) | VOLTAGE_CHANNEL)

#define ADC_PRSCL   0x07    // clk/128

// calculated values. need measurements
// R1 = 910k, R2=200k
#ifndef ADC_44
#define ADC_44 738
#endif
#ifndef ADC_22
#define ADC_22 369
#endif


#define FAST 0xA3           // fast PWM both channels
#define PHASE 0xA1          // phase-correct PWM both channels

#define LAYOUT_DEFINED

#endif
