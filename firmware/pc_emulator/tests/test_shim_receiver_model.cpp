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
 * @file test_shim_receiver_model.cpp
 * @brief Unit tests for receiver model shim.
 */

#include "doctest.h"
#include "receiver_model_shim.hpp"

TEST_SUITE_BEGIN("ReceiverModelShim");

// ============================================================================
// ReceiverModel Construction Tests
// ============================================================================

TEST_CASE("ReceiverModel construction") {
    shim::ReceiverModel rx;

    SUBCASE("Default frequency") {
        CHECK_EQ(rx.target_frequency(), 100000000);  // 100 MHz
    }

    SUBCASE("Default bandwidth") {
        CHECK_EQ(rx.baseband_bandwidth(), 1750000);  // 1.75 MHz
    }

    SUBCASE("Default sample rate") {
        CHECK_EQ(rx.sampling_rate(), 3072000);  // 3.072 MSPS
    }

    SUBCASE("Default gains") {
        CHECK_EQ(rx.lna(), 32);
        CHECK_EQ(rx.vga(), 32);
    }

    SUBCASE("Default RF amp off") {
        CHECK_EQ(rx.rf_amp(), false);
    }

    SUBCASE("Default mode is NFM") {
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::NarrowbandFMAudio);
    }

    SUBCASE("Initially disabled") {
        CHECK_EQ(rx.enabled(), false);
    }
}

// ============================================================================
// Frequency Control Tests
// ============================================================================

TEST_CASE("ReceiverModel frequency control") {
    shim::ReceiverModel rx;

    SUBCASE("Set target frequency") {
        rx.set_target_frequency(446000000);  // 446 MHz
        CHECK_EQ(rx.target_frequency(), 446000000);
    }

    SUBCASE("FM broadcast frequency") {
        rx.set_target_frequency(98100000);  // 98.1 MHz
        CHECK_EQ(rx.target_frequency(), 98100000);
    }

    SUBCASE("VHF frequency") {
        rx.set_target_frequency(145500000);  // 145.5 MHz
        CHECK_EQ(rx.target_frequency(), 145500000);
    }

    SUBCASE("UHF frequency") {
        rx.set_target_frequency(435000000);  // 435 MHz
        CHECK_EQ(rx.target_frequency(), 435000000);
    }

    SUBCASE("1 GHz frequency") {
        rx.set_target_frequency(1000000000LL);
        CHECK_EQ(rx.target_frequency(), 1000000000LL);
    }

    SUBCASE("Frequency step") {
        rx.set_frequency_step(12500);
        CHECK_EQ(rx.frequency_step(), 12500);

        rx.set_frequency_step(25000);
        CHECK_EQ(rx.frequency_step(), 25000);
    }
}

// ============================================================================
// Bandwidth and Sample Rate Tests
// ============================================================================

TEST_CASE("ReceiverModel bandwidth and sample rate") {
    shim::ReceiverModel rx;

    SUBCASE("Set bandwidth") {
        rx.set_baseband_bandwidth(200000);
        CHECK_EQ(rx.baseband_bandwidth(), 200000);
    }

    SUBCASE("Set wide bandwidth") {
        rx.set_baseband_bandwidth(20000000);
        CHECK_EQ(rx.baseband_bandwidth(), 20000000);
    }

    SUBCASE("Set sample rate") {
        rx.set_sampling_rate(2048000);
        CHECK_EQ(rx.sampling_rate(), 2048000);
    }

    SUBCASE("Common sample rates") {
        uint32_t rates[] = {1536000, 2048000, 3072000, 4096000, 6144000};
        for (uint32_t rate : rates) {
            rx.set_sampling_rate(rate);
            CHECK_EQ(rx.sampling_rate(), rate);
        }
    }
}

// ============================================================================
// Gain Control Tests
// ============================================================================

TEST_CASE("ReceiverModel gain control") {
    shim::ReceiverModel rx;

    SUBCASE("LNA gain range") {
        for (uint8_t gain = 0; gain <= 40; gain += 8) {
            rx.set_lna(gain);
            CHECK_EQ(rx.lna(), gain);
        }
    }

    SUBCASE("VGA gain range") {
        for (uint8_t gain = 0; gain <= 62; gain += 2) {
            rx.set_vga(gain);
            CHECK_EQ(rx.vga(), gain);
        }
    }

    SUBCASE("RF amplifier on") {
        rx.set_rf_amp(true);
        CHECK_EQ(rx.rf_amp(), true);
    }

    SUBCASE("RF amplifier off") {
        rx.set_rf_amp(true);
        rx.set_rf_amp(false);
        CHECK_EQ(rx.rf_amp(), false);
    }
}

// ============================================================================
// Mode Tests
// ============================================================================

TEST_CASE("ReceiverModel modulation modes") {
    shim::ReceiverModel rx;

    SUBCASE("AM mode") {
        rx.set_modulation(shim::ReceiverModel::Mode::AMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::AMAudio);
    }

    SUBCASE("NFM mode") {
        rx.set_modulation(shim::ReceiverModel::Mode::NarrowbandFMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::NarrowbandFMAudio);
    }

    SUBCASE("WFM mode") {
        rx.set_modulation(shim::ReceiverModel::Mode::WidebandFMAudio);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::WidebandFMAudio);
    }

    SUBCASE("Spectrum mode") {
        rx.set_modulation(shim::ReceiverModel::Mode::SpectrumAnalysis);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::SpectrumAnalysis);
    }

    SUBCASE("Capture mode") {
        rx.set_modulation(shim::ReceiverModel::Mode::Capture);
        CHECK(rx.modulation() == shim::ReceiverModel::Mode::Capture);
    }
}

// ============================================================================
// Configuration Index Tests
// ============================================================================

TEST_CASE("ReceiverModel configuration indices") {
    shim::ReceiverModel rx;

    SUBCASE("AM configuration") {
        for (uint8_t i = 0; i < 5; i++) {
            rx.set_am_configuration(i);
            CHECK_EQ(rx.am_configuration(), i);
        }
    }

    SUBCASE("NBFM configuration") {
        for (uint8_t i = 0; i < 5; i++) {
            rx.set_nbfm_configuration(i);
            CHECK_EQ(rx.nbfm_configuration(), i);
        }
    }

    SUBCASE("WFM configuration") {
        for (uint8_t i = 0; i < 5; i++) {
            rx.set_wfm_configuration(i);
            CHECK_EQ(rx.wfm_configuration(), i);
        }
    }
}

// ============================================================================
// Squelch Tests
// ============================================================================

TEST_CASE("ReceiverModel squelch") {
    shim::ReceiverModel rx;

    SUBCASE("Default squelch") {
        CHECK_EQ(rx.squelch_level(), 80);
    }

    SUBCASE("Set squelch level") {
        rx.set_squelch_level(50);
        CHECK_EQ(rx.squelch_level(), 50);
    }

    SUBCASE("Squelch range") {
        for (uint8_t level = 0; level <= 100; level += 10) {
            rx.set_squelch_level(level);
            CHECK_EQ(rx.squelch_level(), level);
        }
    }
}

// ============================================================================
// Enable/Disable Tests
// ============================================================================

TEST_CASE("ReceiverModel enable/disable") {
    shim::ReceiverModel rx;

    SUBCASE("Enable receiver") {
        rx.enable();
        CHECK_EQ(rx.enabled(), true);
    }

    SUBCASE("Disable receiver") {
        rx.enable();
        rx.disable();
        CHECK_EQ(rx.enabled(), false);
    }

    SUBCASE("Multiple enable/disable cycles") {
        for (int i = 0; i < 3; i++) {
            rx.enable();
            CHECK_EQ(rx.enabled(), true);
            rx.disable();
            CHECK_EQ(rx.enabled(), false);
        }
    }
}

// ============================================================================
// Global Receiver Model Tests
// ============================================================================

TEST_CASE("Global receiver model") {
    SUBCASE("Get global instance") {
        auto& rx = shim::get_receiver_model();
        // Just verify it returns valid reference
        CHECK(&rx != nullptr);
    }

    SUBCASE("Global instance persists changes") {
        auto& rx1 = shim::get_receiver_model();
        rx1.set_target_frequency(123456789);

        auto& rx2 = shim::get_receiver_model();
        CHECK_EQ(rx2.target_frequency(), 123456789);

        // Restore default
        rx1.set_target_frequency(100000000);
    }
}

// ============================================================================
// Initialize Tests
// ============================================================================

TEST_CASE("ReceiverModel initialization") {
    shim::ReceiverModel rx;

    SUBCASE("Initialize sets up state") {
        rx.initialize();
        // Just verify no crash - implementation specific
        CHECK(true);
    }
}

TEST_SUITE_END();
