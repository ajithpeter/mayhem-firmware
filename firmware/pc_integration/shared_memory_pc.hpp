/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __SHARED_MEMORY_PC_HPP__
#define __SHARED_MEMORY_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include "message_queue_pc.hpp"
#include <cstdint>
#include <cstddef>
#include <atomic>

/**
 * @brief Jammer channel configuration (matching hardware).
 */
struct JammerChannel {
    bool enabled;
    uint64_t center;
    uint32_t width;
    uint32_t duration;
};

/**
 * @brief Hopper channel configuration (matching hardware).
 */
struct HopperChannel {
    bool enabled;
    uint64_t center;
    uint32_t width;
    uint32_t duration;
};

/**
 * @brief Tone definition (matching hardware).
 */
struct ToneDef {
    uint32_t delta;
    uint32_t duration;
};

/**
 * @brief Tone data structure (matching hardware).
 */
struct ToneData {
    ToneDef tone_defs[32];
    uint32_t silence;
    uint8_t message[256];
};

/**
 * @brief PC-compatible SharedMemory structure.
 *
 * This provides the same interface as the hardware SharedMemory
 * but uses PC-compatible threading primitives.
 */
struct SharedMemory {
    static constexpr size_t application_queue_k = 11;
    static constexpr size_t app_local_queue_k = 11;

    // Message queues (PC version uses mutex-based queues)
    MessageQueue application_queue;
    MessageQueue app_local_queue;

    // Baseband message pointer (not used on PC)
    const Message* volatile baseband_message{nullptr};

    // M4 panic message
    char m4_panic_msg[32]{0};

    // Baseband data union (matching hardware)
    union {
        ToneData tones_data;
        struct {
            JammerChannel jammer_channels[80];
            HopperChannel hopper_channels[24];
        } dummy_seperate;
        uint8_t data[512];
    } bb_data{};

    // Baseband ready flag
    std::atomic<bool> baseband_ready{false};

    void clear_baseband_ready() { baseband_ready = false; }
    void set_baseband_ready() { baseband_ready = true; }

    // Performance monitoring
    std::atomic<uint8_t> request_m4_performance_counter{0};
    std::atomic<uint8_t> m4_performance_counter{0};
    std::atomic<uint16_t> m4_stack_usage{0};
    std::atomic<uint32_t> m4_heap_usage{0};
    std::atomic<uint16_t> m4_buffer_missed{0};

    SharedMemory() {
        // Initialize bb_data to zeros
        std::memset(&bb_data, 0, sizeof(bb_data));
    }
};

/**
 * @brief Global shared memory instance.
 *
 * On hardware, this is placed in a specific memory region shared
 * between the M0 and M4 processors. On PC, it's just a regular
 * global variable.
 */
extern SharedMemory& shared_memory;

/**
 * @brief Initialize shared memory for PC emulator.
 */
void init_shared_memory();

#endif // PORTAPACK_PC_EMULATOR

#endif // __SHARED_MEMORY_PC_HPP__
