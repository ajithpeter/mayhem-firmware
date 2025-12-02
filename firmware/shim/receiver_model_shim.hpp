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

#ifndef __RECEIVER_MODEL_SHIM_HPP__
#define __RECEIVER_MODEL_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>

namespace shim {

/**
 * @brief Frequency type (Hz)
 */
using Frequency = int64_t;

/**
 * @brief ReceiverModel shim matching the original receiver_model.hpp interface.
 */
class ReceiverModel {
public:
    enum class Mode : uint8_t {
        AMAudio = 0,
        NarrowbandFMAudio = 1,
        WidebandFMAudio = 2,
        SpectrumAnalysis = 3,
        AMAudioFMApt = 4,
        WFMAudioAMApt = 5,
        Capture = 6,
    };

    ReceiverModel() = default;

    // Frequency control
    Frequency target_frequency() const { return target_frequency_; }
    void set_target_frequency(Frequency f);

    // Bandwidth/sampling
    uint32_t baseband_bandwidth() const { return baseband_bandwidth_; }
    void set_baseband_bandwidth(uint32_t v);

    uint32_t sampling_rate() const { return sampling_rate_; }
    void set_sampling_rate(uint32_t v);

    Frequency frequency_step() const { return frequency_step_; }
    void set_frequency_step(Frequency f) { frequency_step_ = f; }

    // Gains
    uint8_t lna() const { return lna_gain_db_; }
    void set_lna(uint8_t v_db);

    uint8_t vga() const { return vga_gain_db_; }
    void set_vga(uint8_t v_db);

    // RF amplifier
    bool rf_amp() const { return rf_amp_; }
    void set_rf_amp(bool enabled);

    // Mode control
    Mode modulation() const { return mode_; }
    void set_modulation(Mode v);

    // Configuration indices
    uint8_t am_configuration() const { return am_config_index_; }
    void set_am_configuration(uint8_t n) { am_config_index_ = n; }

    uint8_t nbfm_configuration() const { return nbfm_config_index_; }
    void set_nbfm_configuration(uint8_t n) { nbfm_config_index_ = n; }

    uint8_t wfm_configuration() const { return wfm_config_index_; }
    void set_wfm_configuration(uint8_t n) { wfm_config_index_ = n; }

    // Squelch
    uint8_t squelch_level() const { return squelch_level_; }
    void set_squelch_level(uint8_t v) { squelch_level_ = v; }

    // State
    void enable();
    void disable();
    bool enabled() const { return enabled_; }

    void initialize();

private:
    Frequency target_frequency_ = 100000000;  // 100 MHz
    uint32_t baseband_bandwidth_ = 1750000;
    uint32_t sampling_rate_ = 3072000;
    Frequency frequency_step_ = 25000;
    uint8_t lna_gain_db_ = 32;
    uint8_t vga_gain_db_ = 32;
    bool rf_amp_ = false;
    Mode mode_ = Mode::NarrowbandFMAudio;
    uint8_t am_config_index_ = 0;
    uint8_t nbfm_config_index_ = 0;
    uint8_t wfm_config_index_ = 0;
    uint8_t squelch_level_ = 80;
    bool enabled_ = false;
};

/**
 * @brief Get the global receiver model instance.
 * @return Reference to the receiver model
 */
ReceiverModel& get_receiver_model();

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __RECEIVER_MODEL_SHIM_HPP__
