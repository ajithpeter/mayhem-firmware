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

#ifndef __AUDIO_SHIM_HPP__
#define __AUDIO_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <cstddef>

namespace shim {
namespace audio {

/**
 * @brief Audio rate enumeration matching the original audio.hpp
 */
enum class Rate : uint32_t {
    Hz_12000 = 12000,
    Hz_24000 = 24000,
    Hz_48000 = 48000,
};

/**
 * @brief Set the audio sample rate.
 * @param rate Sample rate
 */
void set_rate(Rate rate);

/**
 * @brief Get the current audio sample rate.
 * @return Current rate
 */
Rate get_rate();

/**
 * @brief Set headphone volume.
 * @param volume Volume in dB (-60 to 0)
 */
void headphone_volume(int8_t volume);

/**
 * @brief Get headphone volume.
 * @return Volume in dB
 */
int8_t get_headphone_volume();

namespace output {

/**
 * @brief Start audio output.
 */
void start();

/**
 * @brief Stop audio output.
 */
void stop();

/**
 * @brief Check if audio output is running.
 * @return true if running
 */
bool is_running();

/**
 * @brief Write audio samples to output.
 * @param samples Audio samples
 * @param count Number of samples
 * @return Number of samples written
 */
size_t write(const int16_t* samples, size_t count);

} // namespace output

namespace input {

/**
 * @brief Start audio input.
 */
void start();

/**
 * @brief Stop audio input.
 */
void stop();

/**
 * @brief Check if audio input is running.
 * @return true if running
 */
bool is_running();

/**
 * @brief Read audio samples from input.
 * @param samples Buffer for audio samples
 * @param count Maximum samples to read
 * @return Number of samples read
 */
size_t read(int16_t* samples, size_t count);

} // namespace input

} // namespace audio
} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __AUDIO_SHIM_HPP__
