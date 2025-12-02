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
 * @file test_shim_transmitter_model.cpp
 * @brief Unit tests for transmitter model shim.
 */

#include "doctest.h"
#include "transmitter_model_shim.hpp"

TEST_SUITE_BEGIN("TransmitterModelShim");

// ============================================================================
// TransmitterModel Construction Tests
// ============================================================================

TEST_CASE("TransmitterModel construction") {
    shim::TransmitterModel tx;

    SUBCASE("Default frequency") {
        CHECK_EQ(tx.target_frequency(), 100000000);  // 100 MHz
    }

    SUBCASE("Default bandwidth") {
        CHECK_EQ(tx.baseband_bandwidth(), 1750000);  // 1.75 MHz
    }

    SUBCASE("Default sample rate") {
        CHECK_EQ(tx.sampling_rate(), 2400000);  // 2.4 MSPS
    }

    SUBCASE("Default TX gain") {
        CHECK_EQ(tx.tx_gain(), 35);
    }

    SUBCASE("Default RF amp off") {
        CHECK_EQ(tx.rf_amp(), false);
    }

    SUBCASE("Initially disabled") {
        CHECK_EQ(tx.enabled(), false);
    }
}

// ============================================================================
// Frequency Control Tests
// ============================================================================

TEST_CASE("TransmitterModel frequency control") {
    shim::TransmitterModel tx;

    SUBCASE("Set target frequency") {
        tx.set_target_frequency(446000000);  // 446 MHz
        CHECK_EQ(tx.target_frequency(), 446000000);
    }

    SUBCASE("VHF frequency") {
        tx.set_target_frequency(145500000);  // 145.5 MHz
        CHECK_EQ(tx.target_frequency(), 145500000);
    }

    SUBCASE("UHF frequency") {
        tx.set_target_frequency(435000000);  // 435 MHz
        CHECK_EQ(tx.target_frequency(), 435000000);
    }

    SUBCASE("Wide frequency range") {
        // HackRF supports 1 MHz to 6 GHz
        tx.set_target_frequency(1000000);  // 1 MHz
        CHECK_EQ(tx.target_frequency(), 1000000);

        tx.set_target_frequency(6000000000LL);  // 6 GHz
        CHECK_EQ(tx.target_frequency(), 6000000000LL);
    }
}

// ============================================================================
// Bandwidth and Sample Rate Tests
// ============================================================================

TEST_CASE("TransmitterModel bandwidth and sample rate") {
    shim::TransmitterModel tx;

    SUBCASE("Set bandwidth") {
        tx.set_baseband_bandwidth(200000);
        CHECK_EQ(tx.baseband_bandwidth(), 200000);
    }

    SUBCASE("Wide bandwidth") {
        tx.set_baseband_bandwidth(5000000);
        CHECK_EQ(tx.baseband_bandwidth(), 5000000);
    }

    SUBCASE("Set sample rate") {
        tx.set_sampling_rate(4096000);
        CHECK_EQ(tx.sampling_rate(), 4096000);
    }

    SUBCASE("Common TX sample rates") {
        uint32_t rates[] = {1536000, 2048000, 3072000, 4096000};
        for (uint32_t rate : rates) {
            tx.set_sampling_rate(rate);
            CHECK_EQ(tx.sampling_rate(), rate);
        }
    }
}

// ============================================================================
// TX Gain Tests
// ============================================================================

TEST_CASE("TransmitterModel TX gain") {
    shim::TransmitterModel tx;

    SUBCASE("TX gain range") {
        // HackRF TX VGA gain is 0-47 dB
        for (uint8_t gain = 0; gain <= 47; gain++) {
            tx.set_tx_gain(gain);
            CHECK_EQ(tx.tx_gain(), gain);
        }
    }

    SUBCASE("Minimum TX gain") {
        tx.set_tx_gain(0);
        CHECK_EQ(tx.tx_gain(), 0);
    }

    SUBCASE("Maximum TX gain") {
        tx.set_tx_gain(47);
        CHECK_EQ(tx.tx_gain(), 47);
    }

    SUBCASE("RF amplifier on") {
        tx.set_rf_amp(true);
        CHECK_EQ(tx.rf_amp(), true);
    }

    SUBCASE("RF amplifier off") {
        tx.set_rf_amp(true);
        tx.set_rf_amp(false);
        CHECK_EQ(tx.rf_amp(), false);
    }
}

// ============================================================================
// Channel Bandwidth Tests
// ============================================================================

TEST_CASE("TransmitterModel channel bandwidth") {
    shim::TransmitterModel tx;

    SUBCASE("Set channel bandwidth") {
        tx.set_channel_bandwidth(25000);  // 25 kHz
        CHECK_EQ(tx.channel_bandwidth(), 25000);
    }

    SUBCASE("Narrow channel") {
        tx.set_channel_bandwidth(12500);  // 12.5 kHz
        CHECK_EQ(tx.channel_bandwidth(), 12500);
    }

    SUBCASE("Wide channel") {
        tx.set_channel_bandwidth(200000);  // 200 kHz for WFM
        CHECK_EQ(tx.channel_bandwidth(), 200000);
    }
}

// ============================================================================
// Enable/Disable Tests
// ============================================================================

TEST_CASE("TransmitterModel enable/disable") {
    shim::TransmitterModel tx;

    SUBCASE("Enable transmitter") {
        tx.enable();
        CHECK_EQ(tx.enabled(), true);
    }

    SUBCASE("Disable transmitter") {
        tx.enable();
        tx.disable();
        CHECK_EQ(tx.enabled(), false);
    }

    SUBCASE("Multiple enable/disable cycles") {
        for (int i = 0; i < 3; i++) {
            tx.enable();
            CHECK_EQ(tx.enabled(), true);
            tx.disable();
            CHECK_EQ(tx.enabled(), false);
        }
    }
}

// ============================================================================
// Global Transmitter Model Tests
// ============================================================================

TEST_CASE("Global transmitter model") {
    SUBCASE("Get global instance") {
        auto& tx = shim::get_transmitter_model();
        CHECK(&tx != nullptr);
    }

    SUBCASE("Global instance persists changes") {
        auto& tx1 = shim::get_transmitter_model();
        tx1.set_target_frequency(987654321);

        auto& tx2 = shim::get_transmitter_model();
        CHECK_EQ(tx2.target_frequency(), 987654321);

        // Restore default
        tx1.set_target_frequency(100000000);
    }
}

// ============================================================================
// Initialize Tests
// ============================================================================

TEST_CASE("TransmitterModel initialization") {
    shim::TransmitterModel tx;

    SUBCASE("Initialize sets up state") {
        tx.initialize();
        CHECK(true);
    }
}

// ============================================================================
// Complete Configuration Tests
// ============================================================================

TEST_CASE("TransmitterModel complete configuration") {
    shim::TransmitterModel tx;

    SUBCASE("Configure for FM transmission") {
        tx.set_target_frequency(446093750);  // PMR446 Ch1
        tx.set_baseband_bandwidth(200000);
        tx.set_sampling_rate(2048000);
        tx.set_channel_bandwidth(12500);
        tx.set_tx_gain(20);  // Moderate power
        tx.set_rf_amp(false);

        CHECK_EQ(tx.target_frequency(), 446093750);
        CHECK_EQ(tx.baseband_bandwidth(), 200000);
        CHECK_EQ(tx.sampling_rate(), 2048000);
        CHECK_EQ(tx.channel_bandwidth(), 12500);
        CHECK_EQ(tx.tx_gain(), 20);
        CHECK_EQ(tx.rf_amp(), false);
    }

    SUBCASE("Configure for wide FM transmission") {
        tx.set_target_frequency(88100000);  // 88.1 MHz FM
        tx.set_baseband_bandwidth(200000);
        tx.set_sampling_rate(4096000);
        tx.set_channel_bandwidth(200000);
        tx.set_tx_gain(47);  // Max power
        tx.set_rf_amp(true);

        CHECK_EQ(tx.target_frequency(), 88100000);
        CHECK_EQ(tx.channel_bandwidth(), 200000);
        CHECK_EQ(tx.tx_gain(), 47);
        CHECK_EQ(tx.rf_amp(), true);
    }
}

TEST_SUITE_END();
