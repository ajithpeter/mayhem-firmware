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

#ifndef __HAL_AUDIO_HPP__
#define __HAL_AUDIO_HPP__

#include <cstdint>
#include <cstddef>
#include <functional>

namespace hal {

/**
 * @brief Abstract interface for audio hardware abstraction layer.
 *
 * This interface defines the contract that all audio implementations
 * must fulfill, allowing the application to work with different audio
 * backends (WM8731/AK4951 codec, PortAudio, etc.)
 */
class IAudio {
public:
    // Audio callback for streaming audio
    using AudioCallback = std::function<void(int16_t* buffer, size_t frames)>;

    virtual ~IAudio() = default;

    /**
     * @brief Initialize the audio hardware
     * @return true on success, false on failure
     */
    virtual bool init() = 0;

    /**
     * @brief Shutdown the audio hardware
     */
    virtual void shutdown() = 0;

    /**
     * @brief Set the sample rate for audio I/O
     * @param rate Sample rate in Hz (typically 12000, 24000, or 48000)
     */
    virtual void set_sample_rate(uint32_t rate) = 0;

    /**
     * @brief Get the current sample rate
     * @return Current sample rate in Hz
     */
    virtual uint32_t sample_rate() const = 0;

    /**
     * @brief Set headphone/line-out volume
     * @param volume_db Volume in dB (typically -60 to 0)
     */
    virtual void set_volume(int8_t volume_db) = 0;

    /**
     * @brief Get current volume
     * @return Volume in dB
     */
    virtual int8_t volume() const = 0;

    /**
     * @brief Start audio output
     */
    virtual void output_start() = 0;

    /**
     * @brief Stop audio output
     */
    virtual void output_stop() = 0;

    /**
     * @brief Start audio input (microphone)
     */
    virtual void input_start() = 0;

    /**
     * @brief Stop audio input
     */
    virtual void input_stop() = 0;

    /**
     * @brief Write audio samples to output buffer
     * @param samples Pointer to audio sample data (interleaved stereo or mono)
     * @param count Number of samples to write
     * @return Number of samples actually written
     */
    virtual size_t write_samples(const int16_t* samples, size_t count) = 0;

    /**
     * @brief Read audio samples from input buffer
     * @param samples Pointer to buffer for audio sample data
     * @param count Number of samples to read
     * @return Number of samples actually read
     */
    virtual size_t read_samples(int16_t* samples, size_t count) = 0;

    /**
     * @brief Set callback for audio output (pull model)
     * @param callback Function to call when audio data is needed
     */
    virtual void set_output_callback(AudioCallback callback) = 0;

    /**
     * @brief Set callback for audio input (push model)
     * @param callback Function to call when audio data is available
     */
    virtual void set_input_callback(AudioCallback callback) = 0;

    /**
     * @brief Check if audio output is currently running
     * @return true if output is active
     */
    virtual bool is_output_running() const = 0;

    /**
     * @brief Check if audio input is currently running
     * @return true if input is active
     */
    virtual bool is_input_running() const = 0;
};

} // namespace hal

#endif // __HAL_AUDIO_HPP__
