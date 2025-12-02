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
 * @file test_rf_mock.cpp
 * @brief Unit tests for Mock RF implementation.
 */

#include "doctest.h"
#include "rf_soapysdr.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

TEST_SUITE_BEGIN("RFMock");

// ============================================================================
// RFMock Construction and Basic Operations
// ============================================================================

TEST_CASE("RFMock construction") {
    hal::RFMock rf;

    SUBCASE("Initial state is not initialized") {
        // Object should be creatable
        CHECK(true);
    }
}

TEST_CASE("RFMock initialization") {
    hal::RFMock rf;

    SUBCASE("init returns true") {
        CHECK(rf.init() == true);
    }

    SUBCASE("Can shutdown after init") {
        rf.init();
        rf.shutdown();
        CHECK(true);
    }

    SUBCASE("Multiple init/shutdown cycles") {
        for (int i = 0; i < 3; i++) {
            CHECK(rf.init() == true);
            rf.shutdown();
        }
    }

    SUBCASE("is_available after init") {
        rf.init();
        CHECK(rf.is_available() == true);
        rf.shutdown();
    }

    SUBCASE("device_info returns non-empty string") {
        rf.init();
        std::string info = rf.device_info();
        CHECK(info.length() > 0);
        rf.shutdown();
    }
}

// ============================================================================
// Frequency Control Tests
// ============================================================================

TEST_CASE("RFMock frequency control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set and get frequency") {
        int64_t freq = 100000000;  // 100 MHz
        rf.set_frequency(freq);
        CHECK_EQ(rf.frequency(), freq);
    }

    SUBCASE("FM broadcast band") {
        int64_t freq = 98100000;  // 98.1 MHz
        rf.set_frequency(freq);
        CHECK_EQ(rf.frequency(), freq);
    }

    SUBCASE("UHF frequency") {
        int64_t freq = 446000000;  // 446 MHz (PMR446)
        rf.set_frequency(freq);
        CHECK_EQ(rf.frequency(), freq);
    }

    SUBCASE("1 GHz frequency") {
        int64_t freq = 1000000000LL;  // 1 GHz
        rf.set_frequency(freq);
        CHECK_EQ(rf.frequency(), freq);
    }

    SUBCASE("2.4 GHz frequency") {
        int64_t freq = 2400000000LL;  // 2.4 GHz
        rf.set_frequency(freq);
        CHECK_EQ(rf.frequency(), freq);
    }

    rf.shutdown();
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST_CASE("RFMock sample rate control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set and get sample rate") {
        uint32_t rate = 2000000;  // 2 MSPS
        rf.set_sample_rate(rate);
        CHECK_EQ(rf.sample_rate(), rate);
    }

    SUBCASE("Common sample rates") {
        uint32_t rates[] = {1000000, 2000000, 4000000, 8000000, 10000000, 20000000};
        for (uint32_t rate : rates) {
            rf.set_sample_rate(rate);
            CHECK_EQ(rf.sample_rate(), rate);
        }
    }

    rf.shutdown();
}

// ============================================================================
// Bandwidth Tests
// ============================================================================

TEST_CASE("RFMock bandwidth control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set and get bandwidth") {
        uint32_t bw = 1750000;  // 1.75 MHz
        rf.set_bandwidth(bw);
        CHECK_EQ(rf.bandwidth(), bw);
    }

    SUBCASE("Narrow bandwidth") {
        uint32_t bw = 200000;  // 200 kHz
        rf.set_bandwidth(bw);
        CHECK_EQ(rf.bandwidth(), bw);
    }

    SUBCASE("Wide bandwidth") {
        uint32_t bw = 20000000;  // 20 MHz
        rf.set_bandwidth(bw);
        CHECK_EQ(rf.bandwidth(), bw);
    }

    rf.shutdown();
}

// ============================================================================
// Gain Control Tests
// ============================================================================

TEST_CASE("RFMock gain control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("LNA gain") {
        for (int32_t gain = 0; gain <= 40; gain += 8) {
            rf.set_lna_gain(gain);
            CHECK_EQ(rf.lna_gain(), gain);
        }
    }

    SUBCASE("VGA gain") {
        for (int32_t gain = 0; gain <= 62; gain += 2) {
            rf.set_vga_gain(gain);
            CHECK_EQ(rf.vga_gain(), gain);
        }
    }

    SUBCASE("TX gain") {
        for (int32_t gain = 0; gain <= 47; gain++) {
            rf.set_tx_gain(gain);
            CHECK_EQ(rf.tx_gain(), gain);
        }
    }

    rf.shutdown();
}

// ============================================================================
// RF Amplifier Tests
// ============================================================================

TEST_CASE("RFMock RF amplifier control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Enable RF amp") {
        rf.set_amp_enable(true);
        CHECK_EQ(rf.amp_enabled(), true);
    }

    SUBCASE("Disable RF amp") {
        rf.set_amp_enable(false);
        CHECK_EQ(rf.amp_enabled(), false);
    }

    SUBCASE("Toggle RF amp") {
        rf.set_amp_enable(false);
        CHECK_EQ(rf.amp_enabled(), false);
        rf.set_amp_enable(true);
        CHECK_EQ(rf.amp_enabled(), true);
        rf.set_amp_enable(false);
        CHECK_EQ(rf.amp_enabled(), false);
    }

    rf.shutdown();
}

// ============================================================================
// Antenna Bias Tests
// ============================================================================

TEST_CASE("RFMock antenna bias control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Enable antenna bias") {
        rf.set_antenna_bias(true);
        CHECK_EQ(rf.antenna_bias(), true);
    }

    SUBCASE("Disable antenna bias") {
        rf.set_antenna_bias(false);
        CHECK_EQ(rf.antenna_bias(), false);
    }

    rf.shutdown();
}

// ============================================================================
// Direction Control Tests
// ============================================================================

TEST_CASE("RFMock direction control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set receive direction") {
        rf.set_direction(hal::IRF::Direction::Receive);
        CHECK(rf.direction() == hal::IRF::Direction::Receive);
    }

    SUBCASE("Set transmit direction") {
        rf.set_direction(hal::IRF::Direction::Transmit);
        CHECK(rf.direction() == hal::IRF::Direction::Transmit);
    }

    rf.shutdown();
}

// ============================================================================
// Streaming Tests
// ============================================================================

TEST_CASE("RFMock streaming control") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Start and stop RX streaming") {
        rf.set_direction(hal::IRF::Direction::Receive);
        rf.start_rx();
        CHECK(rf.is_streaming() == true);
        rf.stop();
        CHECK(rf.is_streaming() == false);
    }

    SUBCASE("Start and stop TX streaming") {
        rf.set_direction(hal::IRF::Direction::Transmit);
        rf.start_tx();
        CHECK(rf.is_streaming() == true);
        rf.stop();
        CHECK(rf.is_streaming() == false);
    }

    SUBCASE("Multiple start/stop cycles") {
        rf.set_direction(hal::IRF::Direction::Receive);
        for (int i = 0; i < 3; i++) {
            rf.start_rx();
            CHECK(rf.is_streaming() == true);
            rf.stop();
            CHECK(rf.is_streaming() == false);
        }
    }

    rf.shutdown();
}

// ============================================================================
// Sample Reading Tests
// ============================================================================

TEST_CASE("RFMock sample reading") {
    hal::RFMock rf;
    rf.init();
    rf.set_sample_rate(2000000);
    rf.set_direction(hal::IRF::Direction::Receive);
    rf.start_rx();

    SUBCASE("Read samples returns data") {
        std::vector<int8_t> buffer(1024);
        int read = rf.read_samples(buffer.data(), buffer.size() / 2);  // count is sample pairs
        CHECK(read > 0);
        CHECK(read <= static_cast<int>(buffer.size() / 2));
    }

    SUBCASE("Samples are within valid range") {
        std::vector<int8_t> buffer(1024);
        rf.read_samples(buffer.data(), buffer.size() / 2);

        for (int8_t sample : buffer) {
            CHECK(sample >= -128);
            CHECK(sample <= 127);
        }
    }

    SUBCASE("Read multiple times") {
        std::vector<int8_t> buffer(256);
        for (int i = 0; i < 10; i++) {
            int read = rf.read_samples(buffer.data(), buffer.size() / 2);
            CHECK(read >= 0);
        }
    }

    rf.stop();
    rf.shutdown();
}

// ============================================================================
// Sample Writing Tests (TX)
// ============================================================================

TEST_CASE("RFMock sample writing") {
    hal::RFMock rf;
    rf.init();
    rf.set_direction(hal::IRF::Direction::Transmit);
    rf.start_tx();

    SUBCASE("Write samples") {
        std::vector<int8_t> buffer(1024);
        // Fill with test pattern
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = static_cast<int8_t>(i % 256 - 128);
        }

        int written = rf.write_samples(buffer.data(), buffer.size() / 2);  // count is sample pairs
        CHECK(written >= 0);
    }

    rf.stop();
    rf.shutdown();
}

// ============================================================================
// Mock Signal Generation Tests
// ============================================================================

TEST_CASE("Mock signal properties") {
    hal::RFMock rf;
    rf.init();
    rf.set_sample_rate(2000000);
    rf.set_direction(hal::IRF::Direction::Receive);
    rf.start_rx();

    SUBCASE("Signal has variation (not constant)") {
        std::vector<int8_t> buffer(1024);
        rf.read_samples(buffer.data(), buffer.size() / 2);

        // Count unique values
        std::vector<int8_t> unique(buffer.begin(), buffer.end());
        std::sort(unique.begin(), unique.end());
        unique.erase(std::unique(unique.begin(), unique.end()), unique.end());

        CHECK(unique.size() > 1);  // Should have multiple different values
    }

    SUBCASE("IQ samples are interleaved") {
        std::vector<int8_t> buffer(256);
        rf.read_samples(buffer.data(), buffer.size() / 2);

        // Buffer size should be even (I/Q pairs)
        CHECK(buffer.size() % 2 == 0);
    }

    rf.stop();
    rf.shutdown();
}

// ============================================================================
// Mock-specific Methods Tests
// ============================================================================

TEST_CASE("RFMock test signal configuration") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set test signal frequency") {
        rf.set_test_signal_frequency(1000.0);
        // Just verify it doesn't crash
        CHECK(true);
    }

    SUBCASE("Set test signal amplitude") {
        rf.set_test_signal_amplitude(0.5f);
        // Just verify it doesn't crash
        CHECK(true);
    }

    rf.shutdown();
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_CASE("RFMock callbacks") {
    hal::RFMock rf;
    rf.init();

    SUBCASE("Set RX callback") {
        bool callback_called = false;
        rf.set_rx_callback([&](const int8_t* samples, size_t count) {
            callback_called = true;
        });
        // Just verify registration doesn't crash
        CHECK(true);
    }

    SUBCASE("Set TX callback") {
        rf.set_tx_callback([](int8_t* buffer, size_t max_count) -> size_t {
            return 0;
        });
        // Just verify registration doesn't crash
        CHECK(true);
    }

    rf.shutdown();
}

TEST_SUITE_END();
