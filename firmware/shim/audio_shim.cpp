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

#include "audio_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"

namespace shim {
namespace audio {

static Rate current_rate = Rate::Hz_24000;

void set_rate(Rate rate) {
    current_rate = rate;
    portapack_shim::audio_hal.set_sample_rate(static_cast<uint32_t>(rate));
}

Rate get_rate() {
    return current_rate;
}

void headphone_volume(int8_t volume) {
    portapack_shim::audio_hal.set_volume(volume);
}

int8_t get_headphone_volume() {
    return portapack_shim::audio_hal.volume();
}

namespace output {

void start() {
    portapack_shim::audio_hal.output_start();
}

void stop() {
    portapack_shim::audio_hal.output_stop();
}

bool is_running() {
    return portapack_shim::audio_hal.is_output_running();
}

size_t write(const int16_t* samples, size_t count) {
    return portapack_shim::audio_hal.write_samples(samples, count);
}

} // namespace output

namespace input {

void start() {
    portapack_shim::audio_hal.input_start();
}

void stop() {
    portapack_shim::audio_hal.input_stop();
}

bool is_running() {
    return portapack_shim::audio_hal.is_input_running();
}

size_t read(int16_t* samples, size_t count) {
    return portapack_shim::audio_hal.read_samples(samples, count);
}

} // namespace input

} // namespace audio
} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
