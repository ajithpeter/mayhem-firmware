/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __AUDIO_PORTAUDIO_HPP__
#define __AUDIO_PORTAUDIO_HPP__

#include "../interface/hal_audio.hpp"
#include <portaudio.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <cmath>

namespace hal {

/**
 * @brief PortAudio-based audio implementation for PC emulation.
 *
 * This class implements the IAudio interface using PortAudio,
 * providing audio input/output capabilities on the PC.
 */
class AudioPortAudio : public IAudio {
public:
    static constexpr size_t BUFFER_FRAMES = 256;
    static constexpr size_t RING_BUFFER_SIZE = 8192;

    AudioPortAudio();
    ~AudioPortAudio() override;

    // IAudio interface implementation
    bool init() override;
    void shutdown() override;
    void set_sample_rate(uint32_t rate) override;
    uint32_t sample_rate() const override;
    void set_volume(int8_t volume_db) override;
    int8_t volume() const override;
    void output_start() override;
    void output_stop() override;
    void input_start() override;
    void input_stop() override;
    size_t write_samples(const int16_t* samples, size_t count) override;
    size_t read_samples(int16_t* samples, size_t count) override;
    void set_output_callback(AudioCallback callback) override;
    void set_input_callback(AudioCallback callback) override;
    bool is_output_running() const override;
    bool is_input_running() const override;

private:
    // PortAudio callback functions
    static int output_callback_static(
        const void* input,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );

    static int input_callback_static(
        const void* input,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );

    int output_callback(
        void* output,
        unsigned long frameCount
    );

    int input_callback(
        const void* input,
        unsigned long frameCount
    );

    // PortAudio streams
    PaStream* output_stream_ = nullptr;
    PaStream* input_stream_ = nullptr;

    // Configuration
    uint32_t sample_rate_ = 24000;
    std::atomic<int8_t> volume_db_{-10};
    float volume_linear_ = 0.316f;  // -10 dB

    // Ring buffers for audio data
    std::vector<int16_t> output_buffer_;
    std::atomic<size_t> output_read_pos_{0};
    std::atomic<size_t> output_write_pos_{0};

    std::vector<int16_t> input_buffer_;
    std::atomic<size_t> input_read_pos_{0};
    std::atomic<size_t> input_write_pos_{0};

    // User callbacks
    AudioCallback output_callback_;
    AudioCallback input_callback_;

    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> output_running_{false};
    std::atomic<bool> input_running_{false};

    // Thread safety
    mutable std::mutex mutex_;

    // Helper functions
    float db_to_linear(int8_t db) const {
        return std::pow(10.0f, db / 20.0f);
    }

    size_t ring_buffer_available(size_t read_pos, size_t write_pos, size_t size) const {
        if (write_pos >= read_pos) {
            return write_pos - read_pos;
        }
        return size - read_pos + write_pos;
    }

    size_t ring_buffer_free(size_t read_pos, size_t write_pos, size_t size) const {
        return size - 1 - ring_buffer_available(read_pos, write_pos, size);
    }
};

} // namespace hal

#endif // __AUDIO_PORTAUDIO_HPP__
