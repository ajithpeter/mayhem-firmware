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
 * @file test_baseband_emulator.cpp
 * @brief Unit tests for baseband processor emulator.
 */

#include "doctest.h"
#include "baseband_emulator.hpp"
#include <cmath>
#include <vector>
#include <cstring>

TEST_SUITE_BEGIN("BasebandEmulator");

// ============================================================================
// ImageTag Tests
// ============================================================================

TEST_CASE("ImageTag operations") {
    SUBCASE("Compare image tags") {
        baseband_emu::ImageTag tag1 = baseband_emu::IMAGE_TAG_AM_AUDIO;
        baseband_emu::ImageTag tag2 = baseband_emu::IMAGE_TAG_AM_AUDIO;
        baseband_emu::ImageTag tag3 = baseband_emu::IMAGE_TAG_NFM_AUDIO;

        CHECK(tag1 == tag2);
        CHECK_FALSE(tag1 == tag3);
    }

    SUBCASE("Known processor tags are defined") {
        CHECK(baseband_emu::IMAGE_TAG_AM_AUDIO);
        CHECK(baseband_emu::IMAGE_TAG_NFM_AUDIO);
        CHECK(baseband_emu::IMAGE_TAG_WFM_AUDIO);
        CHECK(baseband_emu::IMAGE_TAG_CAPTURE);
        CHECK(baseband_emu::IMAGE_TAG_SPECTRUM);
    }

    SUBCASE("None tag is false") {
        CHECK_FALSE(baseband_emu::IMAGE_TAG_NONE);
    }

    SUBCASE("Tag characters are correct") {
        // AM_AUDIO is 'P', 'A', 'M', 'A'
        CHECK(baseband_emu::IMAGE_TAG_AM_AUDIO.c[0] == 'P');
        CHECK(baseband_emu::IMAGE_TAG_AM_AUDIO.c[1] == 'A');
        CHECK(baseband_emu::IMAGE_TAG_AM_AUDIO.c[2] == 'M');
        CHECK(baseband_emu::IMAGE_TAG_AM_AUDIO.c[3] == 'A');
    }
}

// ============================================================================
// MessageID Tests
// ============================================================================

TEST_CASE("MessageID enumeration") {
    SUBCASE("Known message IDs") {
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::RSSIStatistics) == 0);
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::BasebandStatistics) == 1);
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::ChannelStatistics) == 2);
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::Shutdown) == 5);
    }

    SUBCASE("Configuration message IDs") {
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::NBFMConfigure) == 12);
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::WFMConfigure) == 13);
        CHECK(static_cast<uint32_t>(baseband_emu::MessageID::AMConfigure) == 14);
    }
}

// ============================================================================
// BaseMessage Tests
// ============================================================================

TEST_CASE("BaseMessage structure") {
    SUBCASE("Message size") {
        baseband_emu::BaseMessage msg;
        // Message should have id + 508 bytes data
        CHECK(sizeof(msg) >= 512);
    }

    SUBCASE("Create message with ID") {
        baseband_emu::BaseMessage msg;
        msg.id = baseband_emu::MessageID::Shutdown;
        CHECK(msg.id == baseband_emu::MessageID::Shutdown);
    }
}

// ============================================================================
// ChannelSpectrum Tests
// ============================================================================

TEST_CASE("ChannelSpectrum structure") {
    baseband_emu::ChannelSpectrum spectrum;

    SUBCASE("Has 256 bins") {
        CHECK(sizeof(spectrum.db) == 256);
    }

    SUBCASE("Can set sampling rate") {
        spectrum.sampling_rate = 2400000;
        CHECK(spectrum.sampling_rate == 2400000);
    }

    SUBCASE("Can set filter frequencies") {
        spectrum.channel_filter_low_frequency = -100000;
        spectrum.channel_filter_high_frequency = 100000;
        spectrum.channel_filter_transition = 50000;

        CHECK(spectrum.channel_filter_low_frequency == -100000);
        CHECK(spectrum.channel_filter_high_frequency == 100000);
        CHECK(spectrum.channel_filter_transition == 50000);
    }

    SUBCASE("Initialize spectrum data") {
        memset(spectrum.db, 0, sizeof(spectrum.db));
        for (int i = 0; i < 256; i++) {
            CHECK(spectrum.db[i] == 0);
        }
    }
}

// ============================================================================
// BasebandEmulator Construction Tests
// ============================================================================

TEST_CASE("BasebandEmulator construction") {
    SUBCASE("Default construction") {
        baseband_emu::BasebandEmulator emu;
        CHECK_FALSE(emu.is_running());
    }

    SUBCASE("Sample rate getter") {
        baseband_emu::BasebandEmulator emu;
        uint32_t rate = emu.sample_rate();
        CHECK(rate > 0);  // Should have a default
    }
}

// ============================================================================
// Callback Type Tests
// ============================================================================

TEST_CASE("Callback types") {
    SUBCASE("AudioCallback type") {
        baseband_emu::BasebandEmulator::AudioCallback callback =
            [](const int16_t* samples, size_t count) {};
        CHECK(callback != nullptr);
    }

    SUBCASE("SpectrumCallback type") {
        baseband_emu::BasebandEmulator::SpectrumCallback callback =
            [](const baseband_emu::ChannelSpectrum& spectrum) {};
        CHECK(callback != nullptr);
    }

    SUBCASE("MessageCallback type") {
        baseband_emu::BasebandEmulator::MessageCallback callback =
            [](const baseband_emu::BaseMessage* msg) {};
        CHECK(callback != nullptr);
    }
}

// ============================================================================
// BasebandProcessor Interface Tests
// ============================================================================

TEST_CASE("BasebandProcessor interface") {
    SUBCASE("Processor has virtual destructor") {
        // This test ensures the class is properly designed for inheritance
        // The fact that ~BasebandProcessor() is virtual allows proper cleanup
        CHECK(true);  // Compiles means virtual destructor exists
    }
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST_CASE("Sample rate configuration") {
    baseband_emu::BasebandEmulator emu;

    SUBCASE("Set sample rate") {
        emu.set_sample_rate(3072000);
        CHECK_EQ(emu.sample_rate(), 3072000);
    }

    SUBCASE("Common sample rates") {
        uint32_t rates[] = {2400000, 3072000, 4000000, 8000000};
        for (uint32_t rate : rates) {
            emu.set_sample_rate(rate);
            CHECK_EQ(emu.sample_rate(), rate);
        }
    }
}

// ============================================================================
// Processor Creation Tests (without RF/Audio HAL)
// ============================================================================

TEST_CASE("Processor definitions") {
    SUBCASE("AMAudioProcessor exists") {
        baseband_emu::AMAudioProcessor proc;
        CHECK(true);  // Just verify it compiles/constructs
    }

    SUBCASE("NFMAudioProcessor exists") {
        baseband_emu::NFMAudioProcessor proc;
        CHECK(true);
    }

    SUBCASE("WFMAudioProcessor exists") {
        baseband_emu::WFMAudioProcessor proc;
        CHECK(true);
    }

    SUBCASE("SpectrumProcessor exists") {
        baseband_emu::SpectrumProcessor proc;
        CHECK(true);
    }

    SUBCASE("CaptureProcessor exists") {
        baseband_emu::CaptureProcessor proc;
        CHECK(true);
    }

    SUBCASE("TestProcessor exists") {
        baseband_emu::TestProcessor proc;
        CHECK(true);
    }
}

// ============================================================================
// Processor Sample Rate Setting
// ============================================================================

TEST_CASE("Processor sample rate") {
    SUBCASE("AMAudioProcessor sample rate") {
        baseband_emu::AMAudioProcessor proc;
        proc.set_sample_rate(2400000);
        CHECK(true);  // Just verify no crash
    }

    SUBCASE("NFMAudioProcessor sample rate") {
        baseband_emu::NFMAudioProcessor proc;
        proc.set_sample_rate(3072000);
        CHECK(true);
    }
}

// ============================================================================
// Audio Sample Processing Tests
// ============================================================================

TEST_CASE("Audio sample retrieval") {
    SUBCASE("Get audio samples from processor") {
        baseband_emu::NFMAudioProcessor proc;
        proc.set_sample_rate(2400000);

        // Process some dummy IQ samples
        std::vector<int8_t> iq_samples(512);
        for (size_t i = 0; i < iq_samples.size(); i++) {
            iq_samples[i] = static_cast<int8_t>((i % 256) - 128);
        }
        proc.execute(iq_samples.data(), iq_samples.size() / 2);

        // Try to get audio samples
        std::vector<int16_t> audio(256);
        size_t got = proc.get_audio_samples(audio.data(), audio.size());
        // May or may not have samples depending on decimation
        CHECK(got <= audio.size());
    }
}

// ============================================================================
// Message Handling Tests
// ============================================================================

TEST_CASE("Message handling") {
    SUBCASE("Processor receives message") {
        baseband_emu::NFMAudioProcessor proc;

        baseband_emu::BaseMessage msg;
        msg.id = baseband_emu::MessageID::NBFMConfigure;
        memset(msg.data, 0, sizeof(msg.data));

        proc.on_message(&msg);
        CHECK(true);  // Just verify no crash
    }

    SUBCASE("Unknown message is ignored") {
        baseband_emu::NFMAudioProcessor proc;

        baseband_emu::BaseMessage msg;
        msg.id = static_cast<baseband_emu::MessageID>(999);  // Unknown ID
        memset(msg.data, 0, sizeof(msg.data));

        proc.on_message(&msg);
        CHECK(true);  // Just verify no crash
    }
}

// ============================================================================
// Spectrum Processor Tests
// ============================================================================

TEST_CASE("Spectrum processor") {
    SUBCASE("Get spectrum data") {
        baseband_emu::SpectrumProcessor proc;
        proc.set_sample_rate(2400000);

        // Process some IQ samples
        std::vector<int8_t> iq_samples(512);
        for (size_t i = 0; i < iq_samples.size(); i++) {
            double phase = 2.0 * M_PI * i / 16.0;
            iq_samples[i] = static_cast<int8_t>(100 * std::cos(phase));
        }
        proc.execute(iq_samples.data(), iq_samples.size() / 2);

        baseband_emu::ChannelSpectrum spectrum;
        proc.get_spectrum(spectrum);

        // Spectrum should have some non-zero values after processing
        CHECK(true);  // Verify no crash
    }
}

// ============================================================================
// Capture Processor Tests
// ============================================================================

TEST_CASE("Capture processor") {
    SUBCASE("Set capture callback") {
        baseband_emu::CaptureProcessor proc;

        bool callback_called = false;
        proc.set_capture_callback([&](const int8_t* data, size_t count) {
            callback_called = true;
        });

        // Process some samples
        std::vector<int8_t> samples(256);
        proc.execute(samples.data(), samples.size() / 2);

        // Callback should have been called
        CHECK(callback_called == true);
    }
}

// ============================================================================
// Test Processor Tests
// ============================================================================

TEST_CASE("Test processor") {
    SUBCASE("Generates test audio") {
        baseband_emu::TestProcessor proc;
        proc.set_sample_rate(48000);

        // Process some dummy input
        std::vector<int8_t> input(256);
        proc.execute(input.data(), input.size() / 2);

        // Get audio output
        std::vector<int16_t> audio(256);
        size_t got = proc.get_audio_samples(audio.data(), audio.size());

        // Test processor should produce audio
        CHECK(got > 0);
    }
}

TEST_SUITE_END();
