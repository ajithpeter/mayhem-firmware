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

#include "receiver_model_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include "baseband_shim.hpp"
#include <iostream>

namespace shim {

static ReceiverModel receiver_model_instance;

ReceiverModel& get_receiver_model() {
    return receiver_model_instance;
}

void ReceiverModel::set_target_frequency(Frequency f) {
    target_frequency_ = f;
    portapack_shim::rf_hal.set_frequency(f);
}

void ReceiverModel::set_baseband_bandwidth(uint32_t v) {
    baseband_bandwidth_ = v;
    portapack_shim::rf_hal.set_bandwidth(v);
}

void ReceiverModel::set_sampling_rate(uint32_t v) {
    sampling_rate_ = v;
    portapack_shim::rf_hal.set_sample_rate(v);
    baseband::set_sample_rate(v);
}

void ReceiverModel::set_lna(uint8_t v_db) {
    lna_gain_db_ = v_db;
    portapack_shim::rf_hal.set_lna_gain(v_db);
}

void ReceiverModel::set_vga(uint8_t v_db) {
    vga_gain_db_ = v_db;
    portapack_shim::rf_hal.set_vga_gain(v_db);
}

void ReceiverModel::set_rf_amp(bool enabled) {
    rf_amp_ = enabled;
    portapack_shim::rf_hal.set_amp_enable(enabled);
}

void ReceiverModel::set_modulation(Mode v) {
    mode_ = v;

    // Load appropriate baseband processor
    switch (v) {
        case Mode::AMAudio:
        case Mode::AMAudioFMApt:
            baseband::run_image(baseband::IMAGE_TAG_AM_AUDIO);
            break;
        case Mode::NarrowbandFMAudio:
            baseband::run_image(baseband::IMAGE_TAG_NFM_AUDIO);
            break;
        case Mode::WidebandFMAudio:
        case Mode::WFMAudioAMApt:
            baseband::run_image(baseband::IMAGE_TAG_WFM_AUDIO);
            break;
        case Mode::SpectrumAnalysis:
            baseband::run_image(baseband::IMAGE_TAG_SPECTRUM);
            break;
        case Mode::Capture:
            baseband::run_image(baseband::IMAGE_TAG_CAPTURE);
            break;
    }
}

void ReceiverModel::enable() {
    if (enabled_) return;

    enabled_ = true;
    portapack_shim::rf_hal.set_direction(hal::IRF::Direction::Receive);
    portapack_shim::rf_hal.start_rx();

    std::cout << "Receiver enabled at " << target_frequency_ / 1000000.0
              << " MHz" << std::endl;
}

void ReceiverModel::disable() {
    if (!enabled_) return;

    enabled_ = false;
    portapack_shim::rf_hal.stop();
    baseband::shutdown();

    std::cout << "Receiver disabled" << std::endl;
}

void ReceiverModel::initialize() {
    target_frequency_ = 100000000;
    baseband_bandwidth_ = 1750000;
    sampling_rate_ = 3072000;
    frequency_step_ = 25000;
    lna_gain_db_ = 32;
    vga_gain_db_ = 32;
    rf_amp_ = false;
    mode_ = Mode::NarrowbandFMAudio;
    squelch_level_ = 80;
    enabled_ = false;
}

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
