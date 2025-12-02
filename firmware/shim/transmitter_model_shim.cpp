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

#include "transmitter_model_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include "baseband_shim.hpp"
#include <iostream>

namespace shim {

static TransmitterModel transmitter_model_instance;

TransmitterModel& get_transmitter_model() {
    return transmitter_model_instance;
}

void TransmitterModel::set_target_frequency(Frequency f) {
    target_frequency_ = f;
    portapack_shim::rf_hal.set_frequency(f);
}

void TransmitterModel::set_baseband_bandwidth(uint32_t v) {
    baseband_bandwidth_ = v;
    portapack_shim::rf_hal.set_bandwidth(v);
}

void TransmitterModel::set_sampling_rate(uint32_t v) {
    sampling_rate_ = v;
    portapack_shim::rf_hal.set_sample_rate(v);
    baseband::set_sample_rate(v);
}

void TransmitterModel::set_tx_gain(uint8_t v_db) {
    tx_gain_db_ = v_db;
    portapack_shim::rf_hal.set_tx_gain(v_db);
}

void TransmitterModel::set_rf_amp(bool enabled) {
    rf_amp_ = enabled;
    portapack_shim::rf_hal.set_amp_enable(enabled);
}

void TransmitterModel::enable() {
    if (enabled_) return;

    enabled_ = true;
    portapack_shim::rf_hal.set_direction(hal::IRF::Direction::Transmit);
    portapack_shim::rf_hal.start_tx();

    std::cout << "Transmitter enabled at " << target_frequency_ / 1000000.0
              << " MHz, gain " << static_cast<int>(tx_gain_db_) << " dB"
              << std::endl;
}

void TransmitterModel::disable() {
    if (!enabled_) return;

    enabled_ = false;
    portapack_shim::rf_hal.stop();
    baseband::shutdown();

    std::cout << "Transmitter disabled" << std::endl;
}

void TransmitterModel::initialize() {
    target_frequency_ = 100000000;
    baseband_bandwidth_ = 1750000;
    channel_bandwidth_ = 1750000;
    sampling_rate_ = 2400000;
    tx_gain_db_ = 35;
    rf_amp_ = false;
    enabled_ = false;
}

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
