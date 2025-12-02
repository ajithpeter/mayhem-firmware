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
 * @file test_shim_audio.cpp
 * @brief Unit tests for audio shim.
 */

#include "doctest.h"
#include "audio_shim.hpp"
#include <cmath>

TEST_SUITE_BEGIN("AudioShim");

// ============================================================================
// Audio Rate Tests
// ============================================================================

TEST_CASE("Audio rate enum") {
    SUBCASE("Rate values are correct") {
        CHECK(static_cast<uint32_t>(shim::audio::Rate::Hz_12000) == 12000);
        CHECK(static_cast<uint32_t>(shim::audio::Rate::Hz_24000) == 24000);
        CHECK(static_cast<uint32_t>(shim::audio::Rate::Hz_48000) == 48000);
    }
}

TEST_CASE("Audio namespace functions") {
    SUBCASE("Set and get sample rate") {
        shim::audio::set_rate(shim::audio::Rate::Hz_48000);
        CHECK(shim::audio::get_rate() == shim::audio::Rate::Hz_48000);

        shim::audio::set_rate(shim::audio::Rate::Hz_24000);
        CHECK(shim::audio::get_rate() == shim::audio::Rate::Hz_24000);

        shim::audio::set_rate(shim::audio::Rate::Hz_12000);
        CHECK(shim::audio::get_rate() == shim::audio::Rate::Hz_12000);
    }
}

// ============================================================================
// Volume Control Tests
// ============================================================================

TEST_CASE("Volume control") {
    SUBCASE("Set headphone volume") {
        shim::audio::headphone_volume(-20);
        CHECK_EQ(shim::audio::get_headphone_volume(), -20);
    }

    SUBCASE("Volume range") {
        // Volume is in dB (-60 to 0)
        for (int8_t vol = -60; vol <= 0; vol += 10) {
            shim::audio::headphone_volume(vol);
            CHECK_EQ(shim::audio::get_headphone_volume(), vol);
        }
    }

    SUBCASE("Minimum volume") {
        shim::audio::headphone_volume(-60);
        CHECK_EQ(shim::audio::get_headphone_volume(), -60);
    }

    SUBCASE("Maximum volume") {
        shim::audio::headphone_volume(0);
        CHECK_EQ(shim::audio::get_headphone_volume(), 0);
    }
}

// ============================================================================
// Audio Output Tests
// ============================================================================

TEST_CASE("Audio output control") {
    SUBCASE("Start and stop output") {
        // Just verify these don't crash - actual audio requires PortAudio init
        shim::audio::output::start();
        shim::audio::output::stop();
        CHECK(true);
    }

    SUBCASE("Multiple start/stop cycles") {
        for (int i = 0; i < 3; i++) {
            shim::audio::output::start();
            shim::audio::output::stop();
        }
        CHECK(true);  // Just verify no crashes
    }
}

// ============================================================================
// Audio Input Tests
// ============================================================================

TEST_CASE("Audio input control") {
    SUBCASE("Start and stop input") {
        // Just verify these don't crash - actual audio requires PortAudio init
        shim::audio::input::start();
        shim::audio::input::stop();
        CHECK(true);
    }

    SUBCASE("Multiple start/stop cycles") {
        for (int i = 0; i < 3; i++) {
            shim::audio::input::start();
            shim::audio::input::stop();
        }
        CHECK(true);  // Just verify no crashes
    }
}

// ============================================================================
// Sample Processing Tests
// ============================================================================

TEST_CASE("Sample processing calculations") {
    SUBCASE("dB to linear conversion") {
        auto db_to_linear = [](float db) -> float {
            return std::pow(10.0f, db / 20.0f);
        };

        CHECK(db_to_linear(0.0f) == doctest::Approx(1.0f));
        CHECK(db_to_linear(-6.0f) == doctest::Approx(0.5f).epsilon(0.01));
        CHECK(db_to_linear(-20.0f) == doctest::Approx(0.1f).epsilon(0.01));
    }

    SUBCASE("Sample clipping") {
        auto clip_sample = [](int32_t sample) -> int16_t {
            if (sample > INT16_MAX) return INT16_MAX;
            if (sample < INT16_MIN) return INT16_MIN;
            return static_cast<int16_t>(sample);
        };

        CHECK_EQ(clip_sample(0), 0);
        CHECK_EQ(clip_sample(32767), 32767);
        CHECK_EQ(clip_sample(-32768), -32768);
        CHECK_EQ(clip_sample(40000), 32767);
        CHECK_EQ(clip_sample(-40000), -32768);
    }

    SUBCASE("Sample mixing") {
        auto mix_samples = [](int16_t a, int16_t b) -> int16_t {
            int32_t sum = static_cast<int32_t>(a) + static_cast<int32_t>(b);
            if (sum > INT16_MAX) return INT16_MAX;
            if (sum < INT16_MIN) return INT16_MIN;
            return static_cast<int16_t>(sum);
        };

        CHECK_EQ(mix_samples(1000, 2000), 3000);
        CHECK_EQ(mix_samples(30000, 10000), 32767);  // Clipped
        CHECK_EQ(mix_samples(-20000, -20000), -32768);  // Clipped
    }
}

// ============================================================================
// Stereo Handling Tests
// ============================================================================

TEST_CASE("Stereo handling") {
    SUBCASE("Mono to stereo") {
        int16_t mono = 1000;
        int16_t left = mono;
        int16_t right = mono;

        CHECK_EQ(left, mono);
        CHECK_EQ(right, mono);
    }

    SUBCASE("Stereo to mono") {
        int16_t left = 1000;
        int16_t right = 2000;
        int16_t mono = static_cast<int16_t>((static_cast<int32_t>(left) + right) / 2);

        CHECK_EQ(mono, 1500);
    }

    SUBCASE("Pan calculation") {
        auto pan = [](int16_t sample, float pan_value) -> std::pair<int16_t, int16_t> {
            // pan_value: -1.0 = full left, 0.0 = center, 1.0 = full right
            float left_gain = (pan_value <= 0.0f) ? 1.0f : (1.0f - pan_value);
            float right_gain = (pan_value >= 0.0f) ? 1.0f : (1.0f + pan_value);

            return {
                static_cast<int16_t>(sample * left_gain),
                static_cast<int16_t>(sample * right_gain)
            };
        };

        auto [left_center, right_center] = pan(1000, 0.0f);
        CHECK_EQ(left_center, 1000);
        CHECK_EQ(right_center, 1000);

        auto [left_hard, right_hard] = pan(1000, -1.0f);
        CHECK_EQ(left_hard, 1000);
        CHECK_EQ(right_hard, 0);
    }
}

// ============================================================================
// Sample Rate Considerations Tests
// ============================================================================

TEST_CASE("Sample rate considerations") {
    SUBCASE("Duration calculation") {
        uint32_t sample_rate = 48000;
        size_t num_samples = 48000;

        double duration_seconds = static_cast<double>(num_samples) / sample_rate;
        CHECK(duration_seconds == doctest::Approx(1.0));
    }

    SUBCASE("Samples for duration") {
        uint32_t sample_rate = 48000;
        double duration_ms = 100.0;

        size_t samples = static_cast<size_t>(sample_rate * (duration_ms / 1000.0));
        CHECK_EQ(samples, 4800);
    }

    SUBCASE("Bytes for samples (stereo 16-bit)") {
        size_t num_samples = 1000;
        size_t bytes = num_samples * 2 * sizeof(int16_t);  // stereo * 16-bit
        CHECK_EQ(bytes, 4000);
    }
}

// ============================================================================
// Audio Format Tests
// ============================================================================

TEST_CASE("Audio format") {
    SUBCASE("16-bit signed integer range") {
        CHECK_EQ(sizeof(int16_t), 2);
        CHECK_EQ(INT16_MIN, -32768);
        CHECK_EQ(INT16_MAX, 32767);
    }

    SUBCASE("Float to int16 conversion") {
        auto float_to_int16 = [](float f) -> int16_t {
            if (f >= 1.0f) return INT16_MAX;
            if (f <= -1.0f) return INT16_MIN;
            return static_cast<int16_t>(f * 32767.0f);
        };

        CHECK_EQ(float_to_int16(0.0f), 0);
        CHECK_EQ(float_to_int16(1.0f), 32767);
        CHECK_EQ(float_to_int16(-1.0f), -32768);
        CHECK_EQ(float_to_int16(0.5f), 16383);
    }

    SUBCASE("Int16 to float conversion") {
        auto int16_to_float = [](int16_t i) -> float {
            return static_cast<float>(i) / 32768.0f;
        };

        CHECK(int16_to_float(0) == doctest::Approx(0.0f));
        CHECK(int16_to_float(32767) == doctest::Approx(1.0f).epsilon(0.001));
        CHECK(int16_to_float(-32768) == doctest::Approx(-1.0f));
    }
}

TEST_SUITE_END();
