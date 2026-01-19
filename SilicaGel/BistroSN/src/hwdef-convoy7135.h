#ifndef HWDEF_CONVOY_7135_H
#define HWDEF_CONVOY_7135_H

/* Convoy 7135 driver layout
 *                 ----
 *           VCC -|1  8|- GND
 *  PWM (1x7135) -|2  7|- OTC
 *  PWM (7x7135) -|3  6|- SWAT (ISP pin)
 *               -|4  5|- Temperature ADC
 *                 ----
 */

#define CAP_PIN     5       // pin P05, OTC
#define CAP_CHANNEL 5       // MUX 05 corresponds with P05

#define PWM_PIN      1       // pin P01, 7x7135 PWM
#define PWM_LVL      PW1D    // PW1D is the output compare register for P01
#define ALT_PWM_PIN  0       // pin P00, 1x7135 PWM
#define ALT_PWM_LVL  PW0D    // PW0D is the output compare register for P00

#define TEMP_PIN 3          // pin P03, temperature ADC
#define TEMP_CHANNEL 3      // MUX 03 corresponds with P03

#define VOLTAGE_CHANNEL 8    // Internal voltage reference ADC channel

#endif
