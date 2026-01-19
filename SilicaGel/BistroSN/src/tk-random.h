// tk-random.h: Smaller pseudo-random function(s).
// Copyright (C) 2015-2023 Selene ToyKeeper (original ATtiny version)
// Copyright (C) 2025 SilicaGel (SN8F5xxx port)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>

uint8_t pgm_rand() {
    static uint16_t offset = 255;
    // loop through ROM space, but avoid the first 256 bytes
    // because the beginning tends to have a big ramp which
    // doesn't look very random at all
    offset = ((offset + 1) & 0x3ff) | 0x0100;
    return *((__code uint8_t *)offset);
}
