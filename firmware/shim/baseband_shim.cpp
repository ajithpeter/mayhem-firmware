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

#include "baseband_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include "shared_memory_shim.hpp"
#include <iostream>

namespace shim {
namespace baseband {

// Image tag definitions
const ImageTag IMAGE_TAG_NONE = {0, 0, 0, 0};
const ImageTag IMAGE_TAG_AM_AUDIO = {'P', 'A', 'M', 'A'};
const ImageTag IMAGE_TAG_NFM_AUDIO = {'P', 'N', 'F', 'M'};
const ImageTag IMAGE_TAG_WFM_AUDIO = {'P', 'W', 'F', 'M'};
const ImageTag IMAGE_TAG_CAPTURE = {'P', 'C', 'A', 'P'};
const ImageTag IMAGE_TAG_REPLAY = {'P', 'R', 'E', 'P'};
const ImageTag IMAGE_TAG_ADSB_RX = {'P', 'A', 'D', 'R'};
const ImageTag IMAGE_TAG_SPECTRUM = {'P', 'S', 'P', 'E'};

// Convert shim ImageTag to baseband_emu ImageTag
static baseband_emu::ImageTag convert_tag(const ImageTag& tag) {
    baseband_emu::ImageTag result;
    result.c[0] = tag.c[0];
    result.c[1] = tag.c[1];
    result.c[2] = tag.c[2];
    result.c[3] = tag.c[3];
    return result;
}

void run_image(const ImageTag& tag) {
    std::cout << "Loading baseband image: " << tag.c[0] << tag.c[1]
              << tag.c[2] << tag.c[3] << std::endl;

    portapack_shim::baseband_emu.load_processor(convert_tag(tag));
}

void shutdown() {
    portapack_shim::baseband_emu.stop();
}

void set_sample_rate(uint32_t sample_rate) {
    portapack_shim::baseband_emu.set_sample_rate(sample_rate);
}

void spectrum_streaming_start() {
    portapack_shim::baseband_emu.spectrum_start();
}

void spectrum_streaming_stop() {
    portapack_shim::baseband_emu.spectrum_stop();
}

void set_afsk(uint32_t baudrate, uint32_t word_length,
              uint32_t trigger_value, bool trigger_word) {
    // Create and send AFSK configuration message
    baseband_emu::BaseMessage msg;
    msg.id = baseband_emu::MessageID::AFSKRxConfigure;

    // Pack configuration into message data
    uint32_t* data = reinterpret_cast<uint32_t*>(msg.data);
    data[0] = baudrate;
    data[1] = word_length;
    data[2] = trigger_value;
    data[3] = trigger_word ? 1 : 0;

    portapack_shim::baseband_emu.send_message(&msg);
}

void set_ook_data(uint32_t stream_length, uint32_t samples_per_bit,
                  uint8_t repeat, uint32_t pause_symbols, uint8_t de_bruijn_length) {
    baseband_emu::BaseMessage msg;
    msg.id = baseband_emu::MessageID::OOKConfigure;

    uint32_t* data = reinterpret_cast<uint32_t*>(msg.data);
    data[0] = stream_length;
    data[1] = samples_per_bit;
    data[2] = repeat;
    data[3] = pause_symbols;
    data[4] = de_bruijn_length;

    portapack_shim::baseband_emu.send_message(&msg);
}

void set_pocsag(int8_t baud_config) {
    baseband_emu::BaseMessage msg;
    msg.id = baseband_emu::MessageID::POCSAGConfigure;
    msg.data[0] = static_cast<uint8_t>(baud_config);

    portapack_shim::baseband_emu.send_message(&msg);
}

void set_adsb() {
    baseband_emu::BaseMessage msg;
    msg.id = baseband_emu::MessageID::ADSBConfigure;

    portapack_shim::baseband_emu.send_message(&msg);
}

void request_audio_beep(uint32_t freq, uint32_t sample_rate, uint32_t duration_ms) {
    (void)freq;
    (void)sample_rate;
    (void)duration_ms;
    // TODO: Implement audio beep generation
}

void request_beep_stop() {
    // TODO: Implement beep stop
}

} // namespace baseband
} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
