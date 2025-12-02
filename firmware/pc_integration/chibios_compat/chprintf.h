/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * chprintf compatibility for PC Emulator
 */

#ifndef __CHPRINTF_H__
#define __CHPRINTF_H__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdio>
#include <cstdarg>

// Dummy BaseSequentialStream for PC compatibility
typedef void BaseSequentialStream;

static inline int chprintf(BaseSequentialStream* chp, const char* fmt, ...) {
    (void)chp;
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
    return result;
}

static inline int chvprintf(BaseSequentialStream* chp, const char* fmt, va_list args) {
    (void)chp;
    return vprintf(fmt, args);
}

static inline int chsnprintf(char* str, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(str, size, fmt, args);
    va_end(args);
    return result;
}

#endif /* PORTAPACK_PC_EMULATOR */

#endif /* __CHPRINTF_H__ */
