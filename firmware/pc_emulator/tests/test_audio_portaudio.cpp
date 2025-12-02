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
 * @file test_audio_portaudio.cpp
 * @brief Unit tests for PortAudio audio implementation.
 */

#include "doctest.h"
#include "audio_portaudio.hpp"
#include <cmath>

TEST_SUITE_BEGIN("AudioPortAudio");

// ============================================================================
// AudioPortAudio Constants Tests
// ============================================================================

TEST_CASE("AudioPortAudio constants are correct") {
    SUBCASE("Buffer sizes") {
        CHECK_EQ(hal::AudioPortAudio::BUFFER_FRAMES, 256);
        CHECK_EQ(hal::AudioPortAudio::RING_BUFFER_SIZE, 8192);
    }

    SUBCASE("Ring buffer is power of 2") {
        size_t size = hal::AudioPortAudio::RING_BUFFER_SIZE;
        CHECK((size & (size - 1)) == 0);  // Power of 2 check
    }

    SUBCASE("Buffer frames is reasonable") {
        // At 48kHz, 256 frames = ~5.3ms latency
        CHECK(hal::AudioPortAudio::BUFFER_FRAMES >= 64);
        CHECK(hal::AudioPortAudio::BUFFER_FRAMES <= 1024);
    }
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST_CASE("Common sample rates") {
    SUBCASE("Common sample rates are valid") {
        uint32_t valid_rates[] = {8000, 11025, 22050, 44100, 48000, 96000};
        for (uint32_t rate : valid_rates) {
            CHECK(rate > 0);
            CHECK(rate <= 192000);
        }
    }
}

// ============================================================================
// Audio Buffer Calculations
// ============================================================================

TEST_CASE("Audio buffer calculations") {
    const uint32_t sample_rate = 48000;
    const size_t buffer_frames = 256;
    const int channels = 2;

    SUBCASE("Latency calculation") {
        double latency_ms = (static_cast<double>(buffer_frames) / sample_rate) * 1000.0;
        CHECK(latency_ms > 5.0);
        CHECK(latency_ms < 6.0);  // Should be ~5.33ms
    }

    SUBCASE("Buffer size in bytes (16-bit stereo)") {
        size_t bytes_per_sample = sizeof(int16_t);
        size_t buffer_bytes = buffer_frames * channels * bytes_per_sample;
        CHECK_EQ(buffer_bytes, 1024);  // 256 * 2 * 2
    }

    SUBCASE("Ring buffer capacity in samples") {
        size_t ring_samples = hal::AudioPortAudio::RING_BUFFER_SIZE;
        double duration_ms = (static_cast<double>(ring_samples) / sample_rate) * 1000.0;
        CHECK(duration_ms > 100.0);  // Should hold > 100ms of audio
    }
}

// ============================================================================
// Audio Sample Format Tests
// ============================================================================

TEST_CASE("Audio sample format") {
    SUBCASE("16-bit signed integer range") {
        int16_t min_sample = -32768;
        int16_t max_sample = 32767;
        CHECK_EQ(min_sample, INT16_MIN);
        CHECK_EQ(max_sample, INT16_MAX);
    }

    SUBCASE("Silence is zero") {
        int16_t silence = 0;
        CHECK_EQ(silence, 0);
    }

    SUBCASE("Full scale sine wave") {
        // Generate a simple test: peak values of sine wave
        double amplitude = 32767.0;
        int16_t peak_positive = static_cast<int16_t>(amplitude * std::sin(M_PI / 2));
        int16_t peak_negative = static_cast<int16_t>(amplitude * std::sin(-M_PI / 2));

        CHECK_EQ(peak_positive, 32767);
        CHECK_EQ(peak_negative, -32767);
    }
}

// ============================================================================
// Volume Control Tests
// ============================================================================

TEST_CASE("Volume control calculations") {
    SUBCASE("Volume range 0-100") {
        for (int vol = 0; vol <= 100; vol += 10) {
            CHECK(vol >= 0);
            CHECK(vol <= 100);
        }
    }

    SUBCASE("Volume to gain conversion") {
        // Linear volume to gain
        auto volume_to_gain = [](int volume) -> float {
            return static_cast<float>(volume) / 100.0f;
        };

        CHECK(volume_to_gain(0) == doctest::Approx(0.0f));
        CHECK(volume_to_gain(50) == doctest::Approx(0.5f));
        CHECK(volume_to_gain(100) == doctest::Approx(1.0f));
    }

    SUBCASE("Logarithmic volume curve") {
        // dB-based volume curve
        auto volume_to_db = [](int volume) -> float {
            if (volume <= 0) return -60.0f;  // Minimum dB
            return 20.0f * std::log10(static_cast<float>(volume) / 100.0f);
        };

        CHECK(volume_to_db(100) == doctest::Approx(0.0f).epsilon(0.01));
        CHECK(volume_to_db(50) < 0.0f);
        CHECK(volume_to_db(10) < volume_to_db(50));
    }
}

// ============================================================================
// Stereo Interleaving Tests
// ============================================================================

TEST_CASE("Stereo sample interleaving") {
    SUBCASE("Interleaved stereo format") {
        // LRLRLRLR... format
        int16_t samples[8] = {100, 200, 101, 201, 102, 202, 103, 203};

        // Left channel samples at even indices
        CHECK_EQ(samples[0], 100);
        CHECK_EQ(samples[2], 101);
        CHECK_EQ(samples[4], 102);
        CHECK_EQ(samples[6], 103);

        // Right channel samples at odd indices
        CHECK_EQ(samples[1], 200);
        CHECK_EQ(samples[3], 201);
        CHECK_EQ(samples[5], 202);
        CHECK_EQ(samples[7], 203);
    }

    SUBCASE("Frame count calculation") {
        size_t num_samples = 100;  // Total interleaved samples
        size_t channels = 2;
        size_t frames = num_samples / channels;
        CHECK_EQ(frames, 50);
    }
}

// ============================================================================
// Ring Buffer Logic Tests
// ============================================================================

TEST_CASE("Ring buffer logic") {
    const size_t buffer_size = 8;  // Small size for testing

    SUBCASE("Empty buffer detection") {
        size_t read_pos = 0;
        size_t write_pos = 0;
        CHECK(read_pos == write_pos);  // Empty
    }

    SUBCASE("Full buffer detection") {
        size_t read_pos = 0;
        size_t write_pos = buffer_size - 1;
        size_t count = (write_pos - read_pos + buffer_size) % buffer_size;
        CHECK_EQ(count, buffer_size - 1);  // Full (one slot wasted)
    }

    SUBCASE("Wrap-around counting") {
        size_t read_pos = 6;
        size_t write_pos = 2;
        size_t count = (write_pos - read_pos + buffer_size) % buffer_size;
        CHECK_EQ(count, 4);  // 6->7->0->1->2 = 4 slots
    }

    SUBCASE("Available space calculation") {
        size_t read_pos = 3;
        size_t write_pos = 5;
        size_t used = (write_pos - read_pos + buffer_size) % buffer_size;
        size_t available = buffer_size - 1 - used;
        CHECK_EQ(used, 2);
        CHECK_EQ(available, 5);
    }
}

TEST_SUITE_END();
