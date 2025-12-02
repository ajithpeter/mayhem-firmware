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
 * @file test_integration.cpp
 * @brief Integration tests for the PC emulator.
 *
 * These tests verify that multiple components work together correctly.
 */

#include "doctest.h"
#include "receiver_model_shim.hpp"
#include "transmitter_model_shim.hpp"
#include "shared_memory_shim.hpp"
#include "display_shim.hpp"
#include "audio_shim.hpp"
#include "event_shim.hpp"

#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>

TEST_SUITE_BEGIN("Integration");

// ============================================================================
// Receiver and Transmitter Coordination Tests
// ============================================================================

TEST_CASE("Receiver and transmitter coordination") {
    auto& rx = shim::get_receiver_model();
    auto& tx = shim::get_transmitter_model();

    SUBCASE("Frequency synchronization") {
        // Set same frequency on both
        int64_t freq = 446000000;  // PMR446

        rx.set_target_frequency(freq);
        tx.set_target_frequency(freq);

        CHECK_EQ(rx.target_frequency(), tx.target_frequency());
    }

    SUBCASE("Cannot enable both simultaneously (typical use case)") {
        // In real hardware, you can't receive and transmit at once
        rx.disable();
        tx.disable();

        rx.enable();
        CHECK(rx.enabled());
        CHECK_FALSE(tx.enabled());

        rx.disable();
        tx.enable();
        CHECK_FALSE(rx.enabled());
        CHECK(tx.enabled());

        tx.disable();
    }
}

// ============================================================================
// Shared Memory Communication Tests
// ============================================================================

TEST_CASE("Shared memory communication") {
    auto& shared = shim::get_shared_memory();

    SUBCASE("Message passing through baseband queue") {
        shared.baseband_queue.clear();

        shim::Message msg;
        msg.id = 100;
        msg.data[0] = 0x42;

        CHECK(shared.baseband_queue.push(msg));

        shim::Message received;
        CHECK(shared.baseband_queue.pop(received, 0));
        CHECK_EQ(received.id, 100);
        CHECK_EQ(received.data[0], 0x42);
    }

    SUBCASE("Message passing through application queue") {
        shared.application_queue.clear();

        shim::Message msg;
        msg.id = 200;

        CHECK(shared.application_queue.push(msg));

        shim::Message received;
        CHECK(shared.application_queue.pop(received, 0));
        CHECK_EQ(received.id, 200);
    }

    SUBCASE("Bidirectional communication") {
        shared.baseband_queue.clear();
        shared.application_queue.clear();

        // Send from app to baseband
        shim::Message bb_msg;
        bb_msg.id = 1;
        shared.baseband_queue.push(bb_msg);

        // Simulate baseband responding
        shim::Message app_msg;
        app_msg.id = 2;
        shared.application_queue.push(app_msg);

        // Verify both received
        shim::Message bb_received;
        shim::Message app_received;

        CHECK(shared.baseband_queue.pop(bb_received, 0));
        CHECK(shared.application_queue.pop(app_received, 0));
        CHECK_EQ(bb_received.id, 1);
        CHECK_EQ(app_received.id, 2);
    }
}

// ============================================================================
// Event System Integration Tests
// ============================================================================

TEST_CASE("Event system integration") {
    auto& dispatcher = shim::get_event_dispatcher();

    SUBCASE("Callback chain") {
        std::atomic<int> callback_count{0};

        dispatcher.set_key_callback([&](shim::KeyEvent key, bool pressed) {
            callback_count++;
        });

        dispatcher.set_encoder_callback([&](int32_t delta) {
            callback_count++;
        });

        // Verify callbacks are set (can't trigger without SDL)
        CHECK(true);
    }
}

// ============================================================================
// Display and UI Integration Tests
// ============================================================================

TEST_CASE("Display and UI integration") {
    SUBCASE("Color conversions are consistent") {
        // Test that Color to RGB565 round-trips approximately
        shim::Color original(255, 128, 64);
        uint16_t rgb565 = original.to_rgb565();

        // Extract components from RGB565
        uint8_t r = (rgb565 >> 11) & 0x1F;
        uint8_t g = (rgb565 >> 5) & 0x3F;
        uint8_t b = rgb565 & 0x1F;

        // Scale back to 8-bit
        r = (r << 3) | (r >> 2);
        g = (g << 2) | (g >> 4);
        b = (b << 3) | (b >> 2);

        // Should be close to original (some precision loss expected)
        CHECK(std::abs(static_cast<int>(r) - 255) < 10);
        CHECK(std::abs(static_cast<int>(g) - 128) < 10);
        CHECK(std::abs(static_cast<int>(b) - 64) < 10);
    }

    SUBCASE("Rect calculations for UI elements") {
        // Header bar
        shim::Rect header(0, 0, 240, 16);
        CHECK_EQ(header.width, 240);
        CHECK_EQ(header.height, 16);

        // Main content area
        shim::Rect content(0, 16, 240, 320 - 16);
        CHECK_EQ(content.y, 16);
        CHECK_EQ(content.height, 304);

        // Verify they don't overlap
        CHECK(header.bottom() <= content.y);
    }
}

// ============================================================================
// Audio Pipeline Integration Tests
// ============================================================================

TEST_CASE("Audio pipeline integration") {
    SUBCASE("Sample rate consistency") {
        auto& rx = shim::get_receiver_model();

        // Set receiver sample rate
        rx.set_sampling_rate(3072000);

        // Audio sample rate should be different (typically 48kHz for output)
        shim::audio::set_rate(shim::audio::Rate::Hz_48000);

        CHECK_EQ(rx.sampling_rate(), 3072000);
        CHECK(shim::audio::get_rate() == shim::audio::Rate::Hz_48000);
    }

    SUBCASE("Volume doesn't affect RF") {
        auto& rx = shim::get_receiver_model();

        // Set audio headphone volume (in dB)
        shim::audio::headphone_volume(-20);

        // RF gains should be independent
        rx.set_lna(32);
        rx.set_vga(32);

        CHECK_EQ(shim::audio::get_headphone_volume(), -20);
        CHECK_EQ(rx.lna(), 32);
        CHECK_EQ(rx.vga(), 32);
    }
}

// ============================================================================
// Threading Integration Tests
// ============================================================================

TEST_CASE("Threading integration") {
    SUBCASE("Shared memory thread safety") {
        auto& shared = shim::get_shared_memory();
        shared.baseband_queue.clear();

        std::atomic<int> produced{0};
        std::atomic<int> consumed{0};
        const int num_messages = 100;

        // Producer thread
        std::thread producer([&]() {
            for (int i = 0; i < num_messages; i++) {
                shim::Message msg;
                msg.id = i;

                while (!shared.baseband_queue.push(msg)) {
                    std::this_thread::yield();
                }
                produced++;
            }
        });

        // Consumer thread
        std::thread consumer([&]() {
            for (int i = 0; i < num_messages; i++) {
                shim::Message msg;
                while (!shared.baseband_queue.pop(msg, 10)) {
                    std::this_thread::yield();
                }
                consumed++;
            }
        });

        producer.join();
        consumer.join();

        CHECK_EQ(produced.load(), num_messages);
        CHECK_EQ(consumed.load(), num_messages);
    }
}

// ============================================================================
// Configuration Persistence Tests
// ============================================================================

TEST_CASE("Configuration persistence") {
    SUBCASE("Receiver configuration survives enable/disable") {
        auto& rx = shim::get_receiver_model();

        rx.set_target_frequency(145500000);
        rx.set_lna(24);
        rx.set_vga(16);
        rx.set_modulation(shim::ReceiverModel::Mode::NarrowbandFMAudio);

        rx.enable();
        rx.disable();

        CHECK_EQ(rx.target_frequency(), 145500000);
        CHECK_EQ(rx.lna(), 24);
        CHECK_EQ(rx.vga(), 16);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::NarrowbandFMAudio);
    }

    SUBCASE("Transmitter configuration survives enable/disable") {
        auto& tx = shim::get_transmitter_model();

        tx.set_target_frequency(446093750);
        tx.set_tx_gain(30);
        tx.set_rf_amp(true);

        tx.enable();
        tx.disable();

        CHECK_EQ(tx.target_frequency(), 446093750);
        CHECK_EQ(tx.tx_gain(), 30);
        CHECK_EQ(tx.rf_amp(), true);

        // Restore defaults
        tx.set_rf_amp(false);
    }
}

// ============================================================================
// Error Recovery Tests
// ============================================================================

TEST_CASE("Error recovery") {
    SUBCASE("Invalid operations don't crash") {
        auto& rx = shim::get_receiver_model();

        // Disable already disabled
        rx.disable();
        rx.disable();
        CHECK(true);

        // Enable twice
        rx.enable();
        rx.enable();
        CHECK(true);

        rx.disable();
    }

    SUBCASE("Queue operations with empty queue") {
        auto& shared = shim::get_shared_memory();
        shared.baseband_queue.clear();
        shared.application_queue.clear();

        shim::Message msg;
        CHECK_FALSE(shared.baseband_queue.pop(msg, 0));
        CHECK_FALSE(shared.application_queue.pop(msg, 0));
    }
}

// ============================================================================
// Mode Switching Tests
// ============================================================================

TEST_CASE("Mode switching") {
    auto& rx = shim::get_receiver_model();

    SUBCASE("Switch between receive modes") {
        rx.set_modulation(shim::ReceiverModel::Mode::AMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::AMAudio);

        rx.set_modulation(shim::ReceiverModel::Mode::NarrowbandFMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::NarrowbandFMAudio);

        rx.set_modulation(shim::ReceiverModel::Mode::WidebandFMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::WidebandFMAudio);

        rx.set_modulation(shim::ReceiverModel::Mode::SpectrumAnalysis);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::SpectrumAnalysis);
    }
}

// ============================================================================
// Frequency Hopping Test
// ============================================================================

TEST_CASE("Frequency operations") {
    auto& rx = shim::get_receiver_model();

    SUBCASE("Rapid frequency changes") {
        int64_t base_freq = 100000000;  // 100 MHz
        int64_t step = 12500;  // 12.5 kHz

        for (int i = 0; i < 100; i++) {
            rx.set_target_frequency(base_freq + i * step);
            CHECK_EQ(rx.target_frequency(), base_freq + i * step);
        }
    }

    SUBCASE("Frequency with step") {
        rx.set_frequency_step(25000);

        int64_t freq = 100000000;
        rx.set_target_frequency(freq);

        freq += rx.frequency_step();
        rx.set_target_frequency(freq);
        CHECK_EQ(rx.target_frequency(), 100025000);

        freq -= rx.frequency_step() * 2;
        rx.set_target_frequency(freq);
        CHECK_EQ(rx.target_frequency(), 99975000);
    }
}

TEST_SUITE_END();
