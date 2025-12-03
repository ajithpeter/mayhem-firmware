/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * PC-compatible ILI9341 display implementation.
 * Provides the global portapack::display instance.
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "lcd_ili9341_pc.hpp"

namespace portapack {

// Global display instance
lcd::ILI9341 display;

} // namespace portapack

#endif // PORTAPACK_PC_EMULATOR
