// Emisar D1 config options for Anduril
#include "hwdef-Emisar_D1.h"
// same as Emisar D4, mostly
#include "cfg-emisar-d4.h"
// the button lights up
#define USE_INDICATOR_LED
// the aux LEDs are behind the main LEDs
#ifdef USE_INDICATOR_LED_WHILE_RAMPING
#undef USE_INDICATOR_LED_WHILE_RAMPING
#endif
// enable blinking indicator LED while off
#define TICK_DURING_STANDBY
#define STANDBY_TICK_SPEED 3  // every 0.128 s
#define USE_FANCIER_BLINKING_INDICATOR
// stop panicking at ~75% power or ~1000 lm (D1 has a decent power-to-thermal-mass ratio)
#ifdef THERM_FASTER_LEVEL
#undef THERM_FASTER_LEVEL
#endif
#define THERM_FASTER_LEVEL (RAMP_SIZE*9/10)  // throttle back faster when high

// no need to be extra-careful on this light
#ifdef THERM_HARD_TURBO_DROP
#undef THERM_HARD_TURBO_DROP
#endif
