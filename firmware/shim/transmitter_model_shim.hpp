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

#ifndef __TRANSMITTER_MODEL_SHIM_HPP__
#define __TRANSMITTER_MODEL_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>

namespace shim {

/**
 * @brief TransmitterModel shim matching the original transmitter_model.hpp interface.
 */
class TransmitterModel {
public:
    using Frequency = int64_t;

    TransmitterModel() = default;

    // Frequency control
    Frequency target_frequency() const { return target_frequency_; }
    void set_target_frequency(Frequency f);

    // Bandwidth/sampling
    uint32_t baseband_bandwidth() const { return baseband_bandwidth_; }
    void set_baseband_bandwidth(uint32_t v);

    uint32_t channel_bandwidth() const { return channel_bandwidth_; }
    void set_channel_bandwidth(uint32_t v) { channel_bandwidth_ = v; }

    uint32_t sampling_rate() const { return sampling_rate_; }
    void set_sampling_rate(uint32_t v);

    // TX gain
    uint8_t tx_gain() const { return tx_gain_db_; }
    void set_tx_gain(uint8_t v_db);

    // RF amplifier
    bool rf_amp() const { return rf_amp_; }
    void set_rf_amp(bool enabled);

    // State
    void enable();
    void disable();
    bool enabled() const { return enabled_; }

    void initialize();

private:
    Frequency target_frequency_ = 100000000;  // 100 MHz
    uint32_t baseband_bandwidth_ = 1750000;
    uint32_t channel_bandwidth_ = 1750000;
    uint32_t sampling_rate_ = 2400000;
    uint8_t tx_gain_db_ = 35;
    bool rf_amp_ = false;
    bool enabled_ = false;
};

/**
 * @brief Get the global transmitter model instance.
 * @return Reference to the transmitter model
 */
TransmitterModel& get_transmitter_model();

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __TRANSMITTER_MODEL_SHIM_HPP__
