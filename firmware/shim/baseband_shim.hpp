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

#ifndef __BASEBAND_SHIM_HPP__
#define __BASEBAND_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>

namespace shim {
namespace baseband {

/**
 * @brief Image tag type matching spi_flash::image_tag_t
 */
struct ImageTag {
    char c[4];

    ImageTag() : c{0, 0, 0, 0} {}
    ImageTag(char c0, char c1, char c2, char c3) : c{c0, c1, c2, c3} {}

    bool operator==(const ImageTag& other) const {
        return c[0] == other.c[0] && c[1] == other.c[1] &&
               c[2] == other.c[2] && c[3] == other.c[3];
    }
};

// Common image tags
extern const ImageTag IMAGE_TAG_NONE;
extern const ImageTag IMAGE_TAG_AM_AUDIO;
extern const ImageTag IMAGE_TAG_NFM_AUDIO;
extern const ImageTag IMAGE_TAG_WFM_AUDIO;
extern const ImageTag IMAGE_TAG_CAPTURE;
extern const ImageTag IMAGE_TAG_REPLAY;
extern const ImageTag IMAGE_TAG_ADSB_RX;
extern const ImageTag IMAGE_TAG_SPECTRUM;

/**
 * @brief Load and run a baseband processor image.
 * @param tag Image tag identifying the processor
 */
void run_image(const ImageTag& tag);

/**
 * @brief Shutdown the current baseband processor.
 */
void shutdown();

/**
 * @brief Set the baseband sample rate.
 * @param sample_rate Sample rate in Hz
 */
void set_sample_rate(uint32_t sample_rate);

/**
 * @brief Start spectrum streaming.
 */
void spectrum_streaming_start();

/**
 * @brief Stop spectrum streaming.
 */
void spectrum_streaming_stop();

/**
 * @brief Configure AFSK reception.
 */
void set_afsk(uint32_t baudrate, uint32_t word_length,
              uint32_t trigger_value, bool trigger_word);

/**
 * @brief Configure OOK transmission.
 */
void set_ook_data(uint32_t stream_length, uint32_t samples_per_bit,
                  uint8_t repeat, uint32_t pause_symbols, uint8_t de_bruijn_length = 0);

/**
 * @brief Configure POCSAG reception.
 */
void set_pocsag(int8_t baud_config = -1);

/**
 * @brief Configure ADSB reception.
 */
void set_adsb();

/**
 * @brief Request an audio beep.
 */
void request_audio_beep(uint32_t freq, uint32_t sample_rate, uint32_t duration_ms);

/**
 * @brief Stop any audio beep.
 */
void request_beep_stop();

} // namespace baseband
} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __BASEBAND_SHIM_HPP__
