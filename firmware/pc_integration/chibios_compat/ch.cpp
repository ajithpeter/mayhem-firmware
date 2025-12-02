/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * ChibiOS Compatibility Layer Implementation for PC Emulator
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "ch.h"

// Global system lock for critical sections
std::mutex g_system_lock;

#endif /* PORTAPACK_PC_EMULATOR */
