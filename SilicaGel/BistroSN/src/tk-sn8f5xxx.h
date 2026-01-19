// tk-sn8f5xxx.h: Sonix SN8F5xxxx portability header.
// Copyright (C) 2015-2023 Selene ToyKeeper (original ATtiny version)
// Copyright (C) 2025 SilicaGel (SN8F5xxx port)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>
#include "sn8f5701.h"

// This helps abstract away the differences between various Sonix MCUs.

/******************** hardware-specific values **************************/
#if defined(MCU_SN8F5701)
    #define F_CPU 800000UL
    #define FLASHSIZE 4096
    #define FLASHPAGE 32
    #define EEPSIZE (4 * FLASHPAGE)
    // Skip one last page, as it contains power-on controller settings
    #define EEPSTART (FLASHSIZE - FLASHPAGE - EEPSIZE)
    #define DELAY_TICKS 570
#else
    #error Hey, you need to define MCU type.
#endif

/******************** I/O pin and register layout ************************/
#if defined(LAYOUT_CONVOY_7135)
    #include "hwdef-convoy7135.h"
#elif defined(LAYOUT_CONVOY_7135_NOCAP)
    #include "hwdef-convoy7135.h"
    #define LAYOUT_NO_OFFTIM3
#else
    #error Hey, you need to define an I/O pin layout.
#endif

/************************* EEPROM emulation ******************************/
inline uint8_t eeprom_read_byte(uint16_t index) {
    return *((__code uint8_t *)(EEPSTART + index));
}

inline void eeprom_write_byte(uint16_t index, uint8_t value) {
    BISP(EEPSTART + index, (uint8_t)&value);
}
