// tk-calibration.h: Attiny calibration header.
// Copyright (C) 2015-2023 Selene ToyKeeper (original ATtiny version)
// Copyright (C) 2025 SilicaGel (SN8F5xxx port)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// This allows using a single set of hardcoded values across multiple projects.

/********************** Voltage ADC calibration **************************/
// These values were measured using RMM's FET+7135.
// See scripts/battcheck/readings.txt for reference values.
// the ADC values we expect for specific voltages
#define ADC_44     136
#define ADC_43     131
#define ADC_42     126
#define ADC_41     120
#define ADC_40     115
#define ADC_39     110
#define ADC_38     105
#define ADC_37     100
#define ADC_36     94
#define ADC_35     89
#define ADC_34     84
#define ADC_33     79
#define ADC_32     74
#define ADC_31     69
#define ADC_30     63
#define ADC_29     58
#define ADC_28     53
#define ADC_27     48
#define ADC_26     43
#define ADC_25     37
#define ADC_24     32
#define ADC_23     27
#define ADC_22     22
#define ADC_21     17
#define ADC_20     11

#define ADC_100p   ADC_42  // the ADC value for 100% full (resting)
#define ADC_75p    ADC_40  // the ADC value for 75% full (resting)
#define ADC_50p    ADC_38  // the ADC value for 50% full (resting)
#define ADC_25p    ADC_35  // the ADC value for 25% full (resting)
#define ADC_0p     ADC_30  // the ADC value for 0% full (resting)
#define ADC_LOW    ADC_30  // When do we start ramping down
#define ADC_CRIT   ADC_27  // When do we shut the light off


/********************** Offtime capacitor calibration ********************/
// Values are between 1 and 255, and can be measured with offtime-cap.c
// See battcheck/otc-readings.txt for reference values.
// These #defines are the edge boundaries, not the center of the target.
#ifdef OFFTIM3
// The OTC value 0.5s after being disconnected from power
// (anything higher than this is a "short press")
#define CAP_SHORT           2335 // Requires 0.1uF + 1.0uF capacitors
// The OTC value 1.5s after being disconnected from power
// Between CAP_MED and CAP_SHORT is a "medium press"
#define CAP_MED             1247 // Requires 0.1uF + 1.0uF capacitors
// Below CAP_MED is a long press
#else
// The OTC value 0.5s after being disconnected from power
// Anything higher than this is a short press, lower is a long press
#define CAP_SHORT           88 // Requires 0.1uF capacitor
#endif
