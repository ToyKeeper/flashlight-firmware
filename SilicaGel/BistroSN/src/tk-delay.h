// tk-delay.h: Smaller, more flexible _delay_ms() functions.
// Copyright (C) 2015-2023 Selene ToyKeeper (original ATtiny version)
// Copyright (C) 2025 SilicaGel (SN8F5xxx port)
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef OWN_DELAY
#include "tk-sn8f5xxx.h"
static inline void _dumb_delay_1ms(void) {
  for (uint16_t t = DELAY_TICKS; t > 0; t--) {
    NOP();
  }
}
#ifdef USE_DELAY_MS
#define delay_ms _delay_ms
void _delay_ms(uint16_t n)
{
    while(n-- > 0) _dumb_delay_1ms();
}
#endif
#ifdef USE_DELAY_S
#define delay_s _delay_s
void _delay_s(void)  // because it saves a bit of ROM space to do it this way
{
  #ifdef USE_DELAY_MS
  _delay_ms(1000);
  #endif
}
#endif
#endif

