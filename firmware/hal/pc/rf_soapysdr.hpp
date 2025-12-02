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

#ifndef __RF_SOAPYSDR_HPP__
#define __RF_SOAPYSDR_HPP__

#include "../interface/hal_rf.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

#ifdef USE_SOAPYSDR
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Types.hpp>
#endif

namespace hal {

#ifdef USE_SOAPYSDR
/**
 * @brief SoapySDR-based RF implementation for PC emulation.
 *
 * This class implements the IRF interface using SoapySDR,
 * allowing the PC emulator to work with any SDR device
 * supported by SoapySDR (HackRF, RTL-SDR, LimeSDR, etc.)
 */
class RFSoapySDR : public IRF {
public:
    static constexpr size_t BUFFER_SIZE = 2048;

    RFSoapySDR();
    ~RFSoapySDR() override;

    // IRF interface implementation
    bool init(const std::string& device_args = "") override;
    void shutdown() override;
    bool is_available() const override;
    std::string device_info() const override;

    void set_frequency(int64_t freq_hz) override;
    int64_t frequency() const override;

    void set_sample_rate(uint32_t rate) override;
    uint32_t sample_rate() const override;

    void set_bandwidth(uint32_t bw_hz) override;
    uint32_t bandwidth() const override;

    void set_lna_gain(int32_t gain_db) override;
    int32_t lna_gain() const override;

    void set_vga_gain(int32_t gain_db) override;
    int32_t vga_gain() const override;

    void set_tx_gain(int32_t gain_db) override;
    int32_t tx_gain() const override;

    void set_amp_enable(bool enable) override;
    bool amp_enabled() const override;

    void set_antenna_bias(bool enable) override;
    bool antenna_bias() const override;

    void set_direction(Direction dir) override;
    Direction direction() const override;

    void start_rx() override;
    void start_tx() override;
    void stop() override;
    bool is_streaming() const override;

    int read_samples(int8_t* buffer, size_t count) override;
    int write_samples(const int8_t* buffer, size_t count) override;

    void set_rx_callback(RxCallback callback) override;
    void set_tx_callback(TxCallback callback) override;

private:
    void rx_thread_func();
    void tx_thread_func();

    // SoapySDR device
    SoapySDR::Device* device_ = nullptr;
    SoapySDR::Stream* rx_stream_ = nullptr;
    SoapySDR::Stream* tx_stream_ = nullptr;

    // Device info
    std::string device_info_;

    // Configuration
    std::atomic<int64_t> frequency_{100000000};     // 100 MHz default
    std::atomic<uint32_t> sample_rate_{2400000};    // 2.4 MHz default
    std::atomic<uint32_t> bandwidth_{1750000};      // 1.75 MHz default
    std::atomic<int32_t> lna_gain_{32};
    std::atomic<int32_t> vga_gain_{32};
    std::atomic<int32_t> tx_gain_{35};
    std::atomic<bool> amp_enabled_{false};
    std::atomic<bool> antenna_bias_{false};
    std::atomic<Direction> direction_{Direction::Receive};

    // Threading
    std::thread rx_thread_;
    std::thread tx_thread_;
    std::atomic<bool> streaming_{false};
    std::atomic<bool> running_{false};

    // Callbacks
    RxCallback rx_callback_;
    TxCallback tx_callback_;

    // Thread safety
    mutable std::mutex mutex_;
    mutable std::mutex stream_mutex_;

    // State
    std::atomic<bool> initialized_{false};
};
#endif // USE_SOAPYSDR

/**
 * @brief Mock RF implementation for testing without hardware.
 *
 * This class provides a mock RF backend that generates test signals
 * when no actual SDR hardware is available.
 */
class RFMock : public IRF {
public:
    RFMock();
    ~RFMock() override;

    // IRF interface implementation
    bool init(const std::string& device_args = "") override;
    void shutdown() override;
    bool is_available() const override;
    std::string device_info() const override;

    void set_frequency(int64_t freq_hz) override;
    int64_t frequency() const override;

    void set_sample_rate(uint32_t rate) override;
    uint32_t sample_rate() const override;

    void set_bandwidth(uint32_t bw_hz) override;
    uint32_t bandwidth() const override;

    void set_lna_gain(int32_t gain_db) override;
    int32_t lna_gain() const override;

    void set_vga_gain(int32_t gain_db) override;
    int32_t vga_gain() const override;

    void set_tx_gain(int32_t gain_db) override;
    int32_t tx_gain() const override;

    void set_amp_enable(bool enable) override;
    bool amp_enabled() const override;

    void set_antenna_bias(bool enable) override;
    bool antenna_bias() const override;

    void set_direction(Direction dir) override;
    Direction direction() const override;

    void start_rx() override;
    void start_tx() override;
    void stop() override;
    bool is_streaming() const override;

    int read_samples(int8_t* buffer, size_t count) override;
    int write_samples(const int8_t* buffer, size_t count) override;

    void set_rx_callback(RxCallback callback) override;
    void set_tx_callback(TxCallback callback) override;

    // Mock-specific methods
    void set_test_signal_frequency(double freq_hz) { test_freq_ = freq_hz; }
    void set_test_signal_amplitude(float amplitude) { test_amplitude_ = amplitude; }
    void load_iq_file(const std::string& path);

private:
    void generate_samples(int8_t* buffer, size_t count);

    // Configuration
    std::atomic<int64_t> frequency_{100000000};
    std::atomic<uint32_t> sample_rate_{2400000};
    std::atomic<uint32_t> bandwidth_{1750000};
    std::atomic<int32_t> lna_gain_{32};
    std::atomic<int32_t> vga_gain_{32};
    std::atomic<int32_t> tx_gain_{35};
    std::atomic<bool> amp_enabled_{false};
    std::atomic<bool> antenna_bias_{false};
    std::atomic<Direction> direction_{Direction::Receive};
    std::atomic<bool> streaming_{false};

    // Test signal generation
    double test_freq_ = 1000.0;  // 1 kHz test tone
    float test_amplitude_ = 0.5f;
    double phase_ = 0.0;

    // IQ file playback
    std::vector<int8_t> iq_data_;
    size_t iq_position_ = 0;
    bool loop_iq_ = true;

    // Callbacks
    RxCallback rx_callback_;
    TxCallback tx_callback_;
};

} // namespace hal

#endif // __RF_SOAPYSDR_HPP__
