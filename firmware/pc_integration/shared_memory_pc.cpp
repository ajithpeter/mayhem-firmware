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

#ifdef PORTAPACK_PC_EMULATOR

#include "shared_memory_pc.hpp"
#include <iostream>

namespace {
    // Static storage for shared memory
    SharedMemory shared_memory_instance;
}

// Global shared memory reference (matching hardware interface)
SharedMemory& shared_memory = shared_memory_instance;

void init_shared_memory() {
    // Reset all queues
    shared_memory.application_queue.reset();
    shared_memory.app_local_queue.reset();

    // Clear baseband message pointer
    shared_memory.baseband_message = nullptr;

    // Clear panic message
    std::memset(shared_memory.m4_panic_msg, 0, sizeof(shared_memory.m4_panic_msg));

    // Clear baseband data
    std::memset(&shared_memory.bb_data, 0, sizeof(shared_memory.bb_data));

    // Reset flags
    shared_memory.baseband_ready = false;
    shared_memory.request_m4_performance_counter = 0;
    shared_memory.m4_performance_counter = 0;
    shared_memory.m4_stack_usage = 0;
    shared_memory.m4_heap_usage = 0;
    shared_memory.m4_buffer_missed = 0;

    std::cout << "SharedMemory initialized (PC emulator)" << std::endl;
}

#endif // PORTAPACK_PC_EMULATOR
