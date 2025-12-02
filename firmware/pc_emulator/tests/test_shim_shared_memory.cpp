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

/**
 * @file test_shim_shared_memory.cpp
 * @brief Unit tests for shared memory shim.
 */

#include "doctest.h"
#include "shared_memory_shim.hpp"
#include <thread>
#include <chrono>
#include <atomic>

TEST_SUITE_BEGIN("SharedMemoryShim");

// ============================================================================
// ThreadSafeMessageQueue Tests
// ============================================================================

TEST_CASE("ThreadSafeMessageQueue basic operations") {
    shim::ThreadSafeMessageQueue<int, 8> queue;

    SUBCASE("Initial state is empty") {
        CHECK(queue.is_empty());
        CHECK_EQ(queue.size(), 0);
    }

    SUBCASE("Push and pop single element") {
        CHECK(queue.push(42));
        CHECK_FALSE(queue.is_empty());
        CHECK_EQ(queue.size(), 1);

        int value;
        CHECK(queue.pop(value, 0));
        CHECK_EQ(value, 42);
        CHECK(queue.is_empty());
    }

    SUBCASE("Push multiple elements") {
        for (int i = 0; i < 5; i++) {
            CHECK(queue.push(i));
        }
        CHECK_EQ(queue.size(), 5);
    }

    SUBCASE("Pop in FIFO order") {
        for (int i = 0; i < 5; i++) {
            queue.push(i);
        }

        for (int i = 0; i < 5; i++) {
            int value;
            CHECK(queue.pop(value, 0));
            CHECK_EQ(value, i);
        }
    }

    SUBCASE("Pop from empty queue fails") {
        int value;
        CHECK_FALSE(queue.pop(value, 0));  // No timeout
    }
}

TEST_CASE("ThreadSafeMessageQueue capacity") {
    shim::ThreadSafeMessageQueue<int, 4> queue;

    SUBCASE("Fill to capacity") {
        for (int i = 0; i < 4; i++) {
            CHECK(queue.push(i));
        }
        CHECK_EQ(queue.size(), 4);
    }

    SUBCASE("Push to full queue fails") {
        for (int i = 0; i < 4; i++) {
            queue.push(i);
        }
        CHECK_FALSE(queue.push(99));  // Should fail
    }

    SUBCASE("Pop makes room for more") {
        for (int i = 0; i < 4; i++) {
            queue.push(i);
        }

        int value;
        queue.pop(value, 0);
        CHECK(queue.push(99));  // Should succeed now
    }
}

TEST_CASE("ThreadSafeMessageQueue with timeout") {
    shim::ThreadSafeMessageQueue<int, 4> queue;

    SUBCASE("Pop with timeout on empty queue") {
        auto start = std::chrono::steady_clock::now();
        int value;
        CHECK_FALSE(queue.pop(value, 50));  // 50ms timeout
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        CHECK(elapsed >= 40);
        CHECK(elapsed < 150);
    }

    SUBCASE("Pop with data available returns immediately") {
        queue.push(42);

        auto start = std::chrono::steady_clock::now();
        int value;
        CHECK(queue.pop(value, 1000));  // Long timeout
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        CHECK(elapsed < 50);
        CHECK_EQ(value, 42);
    }
}

TEST_CASE("ThreadSafeMessageQueue thread safety") {
    shim::ThreadSafeMessageQueue<int, 256> queue;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    SUBCASE("Producer-consumer pattern") {
        const int num_items = 100;

        // Producer thread
        std::thread producer([&]() {
            for (int i = 0; i < num_items; i++) {
                while (!queue.push(i)) {
                    std::this_thread::yield();
                }
                produced++;
            }
        });

        // Consumer thread
        std::thread consumer([&]() {
            for (int i = 0; i < num_items; i++) {
                int value;
                while (!queue.pop(value, 10)) {
                    std::this_thread::yield();
                }
                consumed++;
            }
        });

        producer.join();
        consumer.join();

        CHECK_EQ(produced.load(), num_items);
        CHECK_EQ(consumed.load(), num_items);
        CHECK(queue.is_empty());
    }
}

TEST_CASE("ThreadSafeMessageQueue clear") {
    shim::ThreadSafeMessageQueue<int, 8> queue;

    SUBCASE("Clear empties the queue") {
        for (int i = 0; i < 5; i++) {
            queue.push(i);
        }
        CHECK_EQ(queue.size(), 5);

        queue.clear();
        CHECK(queue.is_empty());
        CHECK_EQ(queue.size(), 0);
    }
}

// ============================================================================
// SharedMemory Structure Tests
// ============================================================================

TEST_CASE("SharedMemory structure") {
    SUBCASE("Get shared memory instance") {
        auto& shared = shim::get_shared_memory();
        CHECK(&shared != nullptr);
    }

    SUBCASE("Same instance returned") {
        auto& shared1 = shim::get_shared_memory();
        auto& shared2 = shim::get_shared_memory();
        CHECK(&shared1 == &shared2);
    }
}

// ============================================================================
// Message Queue Tests
// ============================================================================

TEST_CASE("SharedMemory message queues") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("Baseband queue operations") {
        shared.baseband_queue.clear();
        CHECK(shared.baseband_queue.is_empty());

        shim::Message msg;
        msg.id = 100;
        CHECK(shared.baseband_queue.push(msg));

        shim::Message received;
        CHECK(shared.baseband_queue.pop(received, 0));
        CHECK_EQ(received.id, 100);
    }

    SUBCASE("Application queue operations") {
        shared.application_queue.clear();
        CHECK(shared.application_queue.is_empty());

        shim::Message msg;
        msg.id = 200;
        CHECK(shared.application_queue.push(msg));

        shim::Message received;
        CHECK(shared.application_queue.pop(received, 0));
        CHECK_EQ(received.id, 200);
    }
}

// ============================================================================
// ChannelSpectrum Tests
// ============================================================================

TEST_CASE("ChannelSpectrum structure") {
    shim::ChannelSpectrum spectrum;

    SUBCASE("Has 256 bins") {
        CHECK_EQ(spectrum.db.size(), 256);
    }

    SUBCASE("Set sampling rate") {
        spectrum.sampling_rate = 2400000;
        CHECK_EQ(spectrum.sampling_rate, 2400000);
    }

    SUBCASE("Set filter frequencies") {
        spectrum.channel_filter_low_frequency = -100000;
        spectrum.channel_filter_high_frequency = 100000;
        spectrum.channel_filter_transition = 10000;

        CHECK_EQ(spectrum.channel_filter_low_frequency, -100000);
        CHECK_EQ(spectrum.channel_filter_high_frequency, 100000);
        CHECK_EQ(spectrum.channel_filter_transition, 10000);
    }
}

// ============================================================================
// JammerChannel Tests
// ============================================================================

TEST_CASE("JammerChannel structure") {
    shim::JammerChannel channel;

    SUBCASE("Default values") {
        CHECK_EQ(channel.enabled, false);
        CHECK_EQ(channel.center_freq, 0);
        CHECK_EQ(channel.bandwidth, 0);
    }

    SUBCASE("Set values") {
        channel.enabled = true;
        channel.center_freq = 433920000;
        channel.bandwidth = 500000;
        channel.duration = 1000;

        CHECK(channel.enabled);
        CHECK_EQ(channel.center_freq, 433920000);
        CHECK_EQ(channel.bandwidth, 500000);
        CHECK_EQ(channel.duration, 1000);
    }
}

// ============================================================================
// HopperChannel Tests
// ============================================================================

TEST_CASE("HopperChannel structure") {
    shim::HopperChannel channel;

    SUBCASE("Default values") {
        CHECK_EQ(channel.frequency, 0);
        CHECK_EQ(channel.duration, 0);
    }

    SUBCASE("Set values") {
        channel.frequency = 446000000;
        channel.duration = 50;

        CHECK_EQ(channel.frequency, 446000000);
        CHECK_EQ(channel.duration, 50);
    }
}

// ============================================================================
// ToneData Tests
// ============================================================================

TEST_CASE("ToneData structure") {
    shim::ToneData tone;

    SUBCASE("Default values") {
        CHECK_EQ(tone.frequency, 0);
        CHECK_EQ(tone.duration, 0);
        CHECK_EQ(tone.delta, 0);
    }

    SUBCASE("Set CTCSS tone") {
        tone.frequency = 885;  // 88.5 Hz CTCSS
        tone.duration = 500;
        tone.delta = 100;

        CHECK_EQ(tone.frequency, 885);
        CHECK_EQ(tone.duration, 500);
        CHECK_EQ(tone.delta, 100);
    }
}

// ============================================================================
// Message Structure Tests
// ============================================================================

TEST_CASE("Message structure") {
    shim::Message msg;

    SUBCASE("Default ID is 0") {
        CHECK_EQ(msg.id, 0);
    }

    SUBCASE("Data array is zeroed") {
        for (size_t i = 0; i < sizeof(msg.data); i++) {
            CHECK_EQ(msg.data[i], 0);
        }
    }

    SUBCASE("Set message ID") {
        msg.id = 12345;
        CHECK_EQ(msg.id, 12345);
    }

    SUBCASE("Set message data") {
        msg.data[0] = 0xFF;
        msg.data[507] = 0xAA;
        CHECK_EQ(msg.data[0], 0xFF);
        CHECK_EQ(msg.data[507], 0xAA);
    }
}

// ============================================================================
// Atomic State Variables Tests
// ============================================================================

TEST_CASE("SharedMemory atomic variables") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("M4 state") {
        shared.m4_state = 42;
        CHECK_EQ(shared.m4_state.load(), 42);
    }

    SUBCASE("Counters") {
        shared.application_counter = 100;
        shared.baseband_counter = 200;

        CHECK_EQ(shared.application_counter.load(), 100);
        CHECK_EQ(shared.baseband_counter.load(), 200);
    }

    SUBCASE("Baseband ready") {
        shared.baseband_ready = true;
        CHECK(shared.baseband_ready.load());

        shared.baseband_ready = false;
        CHECK_FALSE(shared.baseband_ready.load());
    }
}

// ============================================================================
// BB Data Buffer Tests
// ============================================================================

TEST_CASE("SharedMemory bb_data buffer") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("Buffer size") {
        CHECK_EQ(shared.bb_data.size(), 32768);
    }

    SUBCASE("Write and read") {
        shared.bb_data[0] = 0xAA;
        shared.bb_data[100] = 0xBB;
        shared.bb_data[32767] = 0xCC;

        CHECK_EQ(shared.bb_data[0], 0xAA);
        CHECK_EQ(shared.bb_data[100], 0xBB);
        CHECK_EQ(shared.bb_data[32767], 0xCC);
    }
}

// ============================================================================
// Jammer Channels Array Tests
// ============================================================================

TEST_CASE("SharedMemory jammer channels") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("Has 80 jammer channels") {
        CHECK_EQ(shared.jammer_channels.size(), 80);
    }

    SUBCASE("Configure jammer channel") {
        shared.jammer_channels[0].enabled = true;
        shared.jammer_channels[0].center_freq = 433920000;
        shared.jammer_channels[0].bandwidth = 1000000;

        CHECK(shared.jammer_channels[0].enabled);
        CHECK_EQ(shared.jammer_channels[0].center_freq, 433920000);
        CHECK_EQ(shared.jammer_channels[0].bandwidth, 1000000);
    }
}

// ============================================================================
// Hopper Channels Array Tests
// ============================================================================

TEST_CASE("SharedMemory hopper channels") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("Has 24 hopper channels") {
        CHECK_EQ(shared.hopper_channels.size(), 24);
    }

    SUBCASE("Configure hopper channel") {
        shared.hopper_channels[0].frequency = 446000000;
        shared.hopper_channels[0].duration = 100;

        CHECK_EQ(shared.hopper_channels[0].frequency, 446000000);
        CHECK_EQ(shared.hopper_channels[0].duration, 100);
    }
}

TEST_SUITE_END();
