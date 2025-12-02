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

#include "rf_soapysdr.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace hal {

// ============================================================================
// RFSoapySDR Implementation
// ============================================================================

RFSoapySDR::RFSoapySDR() = default;

RFSoapySDR::~RFSoapySDR() {
    shutdown();
}

bool RFSoapySDR::init(const std::string& device_args) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    try {
        // Enumerate devices
        auto results = SoapySDR::Device::enumerate(device_args);
        if (results.empty()) {
            std::cerr << "No SoapySDR devices found" << std::endl;
            return false;
        }

        // Print available devices
        std::cout << "Found " << results.size() << " SoapySDR device(s):" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  [" << i << "] ";
            for (const auto& kv : results[i]) {
                std::cout << kv.first << "=" << kv.second << ", ";
            }
            std::cout << std::endl;
        }

        // Create device (use first found)
        device_ = SoapySDR::Device::make(results[0]);
        if (!device_) {
            std::cerr << "Failed to create SoapySDR device" << std::endl;
            return false;
        }

        // Build device info string
        device_info_ = "SoapySDR: ";
        if (results[0].count("driver")) {
            device_info_ += results[0].at("driver");
        }
        if (results[0].count("label")) {
            device_info_ += " (" + results[0].at("label") + ")";
        }

        // Apply initial configuration
        device_->setSampleRate(SOAPY_SDR_RX, 0, sample_rate_);
        device_->setSampleRate(SOAPY_SDR_TX, 0, sample_rate_);
        device_->setFrequency(SOAPY_SDR_RX, 0, static_cast<double>(frequency_));
        device_->setFrequency(SOAPY_SDR_TX, 0, static_cast<double>(frequency_));
        device_->setBandwidth(SOAPY_SDR_RX, 0, bandwidth_);
        device_->setBandwidth(SOAPY_SDR_TX, 0, bandwidth_);

        // Set gains if available
        auto rx_gains = device_->listGains(SOAPY_SDR_RX, 0);
        for (const auto& name : rx_gains) {
            if (name == "LNA" || name == "RF") {
                device_->setGain(SOAPY_SDR_RX, 0, name, lna_gain_);
            } else if (name == "VGA" || name == "IF" || name == "BB") {
                device_->setGain(SOAPY_SDR_RX, 0, name, vga_gain_);
            }
        }

        initialized_ = true;
        std::cout << "RFSoapySDR initialized: " << device_info_ << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "SoapySDR initialization error: " << e.what() << std::endl;
        return false;
    }
}

void RFSoapySDR::shutdown() {
    stop();

    std::lock_guard<std::mutex> lock(mutex_);

    if (device_) {
        SoapySDR::Device::unmake(device_);
        device_ = nullptr;
    }

    initialized_ = false;
}

bool RFSoapySDR::is_available() const {
    return initialized_ && device_ != nullptr;
}

std::string RFSoapySDR::device_info() const {
    return device_info_;
}

void RFSoapySDR::set_frequency(int64_t freq_hz) {
    frequency_ = freq_hz;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_->setFrequency(SOAPY_SDR_RX, 0, static_cast<double>(freq_hz));
        device_->setFrequency(SOAPY_SDR_TX, 0, static_cast<double>(freq_hz));
    }
}

int64_t RFSoapySDR::frequency() const {
    return frequency_;
}

void RFSoapySDR::set_sample_rate(uint32_t rate) {
    sample_rate_ = rate;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_->setSampleRate(SOAPY_SDR_RX, 0, rate);
        device_->setSampleRate(SOAPY_SDR_TX, 0, rate);
    }
}

uint32_t RFSoapySDR::sample_rate() const {
    return sample_rate_;
}

void RFSoapySDR::set_bandwidth(uint32_t bw_hz) {
    bandwidth_ = bw_hz;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_->setBandwidth(SOAPY_SDR_RX, 0, bw_hz);
        device_->setBandwidth(SOAPY_SDR_TX, 0, bw_hz);
    }
}

uint32_t RFSoapySDR::bandwidth() const {
    return bandwidth_;
}

void RFSoapySDR::set_lna_gain(int32_t gain_db) {
    lna_gain_ = gain_db;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            device_->setGain(SOAPY_SDR_RX, 0, "LNA", gain_db);
        } catch (...) {
            // Try generic gain if LNA not available
            try {
                device_->setGain(SOAPY_SDR_RX, 0, "RF", gain_db);
            } catch (...) {}
        }
    }
}

int32_t RFSoapySDR::lna_gain() const {
    return lna_gain_;
}

void RFSoapySDR::set_vga_gain(int32_t gain_db) {
    vga_gain_ = gain_db;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            device_->setGain(SOAPY_SDR_RX, 0, "VGA", gain_db);
        } catch (...) {
            try {
                device_->setGain(SOAPY_SDR_RX, 0, "IF", gain_db);
            } catch (...) {}
        }
    }
}

int32_t RFSoapySDR::vga_gain() const {
    return vga_gain_;
}

void RFSoapySDR::set_tx_gain(int32_t gain_db) {
    tx_gain_ = gain_db;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        device_->setGain(SOAPY_SDR_TX, 0, gain_db);
    }
}

int32_t RFSoapySDR::tx_gain() const {
    return tx_gain_;
}

void RFSoapySDR::set_amp_enable(bool enable) {
    amp_enabled_ = enable;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            device_->writeSetting("amp_enable", enable ? "true" : "false");
        } catch (...) {}
    }
}

bool RFSoapySDR::amp_enabled() const {
    return amp_enabled_;
}

void RFSoapySDR::set_antenna_bias(bool enable) {
    antenna_bias_ = enable;
    if (device_) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            device_->writeSetting("bias_tee", enable ? "true" : "false");
        } catch (...) {}
    }
}

bool RFSoapySDR::antenna_bias() const {
    return antenna_bias_;
}

void RFSoapySDR::set_direction(Direction dir) {
    direction_ = dir;
}

IRF::Direction RFSoapySDR::direction() const {
    return direction_;
}

void RFSoapySDR::start_rx() {
    if (!device_ || streaming_) return;

    std::lock_guard<std::mutex> lock(stream_mutex_);

    try {
        // Setup RX stream with CS8 format (complex signed 8-bit)
        rx_stream_ = device_->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CS8);
        device_->activateStream(rx_stream_);

        running_ = true;
        streaming_ = true;

        rx_thread_ = std::thread(&RFSoapySDR::rx_thread_func, this);

        std::cout << "RX streaming started" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start RX: " << e.what() << std::endl;
    }
}

void RFSoapySDR::start_tx() {
    if (!device_ || streaming_) return;

    std::lock_guard<std::mutex> lock(stream_mutex_);

    try {
        tx_stream_ = device_->setupStream(SOAPY_SDR_TX, SOAPY_SDR_CS8);
        device_->activateStream(tx_stream_);

        running_ = true;
        streaming_ = true;

        tx_thread_ = std::thread(&RFSoapySDR::tx_thread_func, this);

        std::cout << "TX streaming started" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start TX: " << e.what() << std::endl;
    }
}

void RFSoapySDR::stop() {
    running_ = false;

    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    if (tx_thread_.joinable()) {
        tx_thread_.join();
    }

    std::lock_guard<std::mutex> lock(stream_mutex_);

    if (rx_stream_) {
        device_->deactivateStream(rx_stream_);
        device_->closeStream(rx_stream_);
        rx_stream_ = nullptr;
    }
    if (tx_stream_) {
        device_->deactivateStream(tx_stream_);
        device_->closeStream(tx_stream_);
        tx_stream_ = nullptr;
    }

    streaming_ = false;
}

bool RFSoapySDR::is_streaming() const {
    return streaming_;
}

int RFSoapySDR::read_samples(int8_t* buffer, size_t count) {
    if (!rx_stream_) return -1;

    void* buffs[] = { buffer };
    int flags;
    long long time_ns;

    return device_->readStream(rx_stream_, buffs, count, flags, time_ns, 100000);
}

int RFSoapySDR::write_samples(const int8_t* buffer, size_t count) {
    if (!tx_stream_) return -1;

    const void* buffs[] = { buffer };
    int flags = 0;

    return device_->writeStream(tx_stream_, buffs, count, flags);
}

void RFSoapySDR::set_rx_callback(RxCallback callback) {
    rx_callback_ = callback;
}

void RFSoapySDR::set_tx_callback(TxCallback callback) {
    tx_callback_ = callback;
}

void RFSoapySDR::rx_thread_func() {
    std::vector<int8_t> buffer(BUFFER_SIZE * 2);  // IQ pairs

    while (running_) {
        void* buffs[] = { buffer.data() };
        int flags;
        long long time_ns;

        int ret = device_->readStream(rx_stream_, buffs, BUFFER_SIZE,
                                       flags, time_ns, 100000);

        if (ret > 0 && rx_callback_) {
            rx_callback_(buffer.data(), ret);
        }
    }
}

void RFSoapySDR::tx_thread_func() {
    std::vector<int8_t> buffer(BUFFER_SIZE * 2);

    while (running_) {
        size_t count = 0;
        if (tx_callback_) {
            count = tx_callback_(buffer.data(), BUFFER_SIZE);
        }

        if (count > 0) {
            const void* buffs[] = { buffer.data() };
            int flags = 0;
            device_->writeStream(tx_stream_, buffs, count, flags);
        }
    }
}

// ============================================================================
// RFMock Implementation
// ============================================================================

RFMock::RFMock() = default;

RFMock::~RFMock() {
    shutdown();
}

bool RFMock::init(const std::string& device_args) {
    (void)device_args;
    std::cout << "RFMock initialized (no hardware)" << std::endl;
    return true;
}

void RFMock::shutdown() {
    streaming_ = false;
}

bool RFMock::is_available() const {
    return true;  // Mock is always available
}

std::string RFMock::device_info() const {
    return "Mock RF (no hardware)";
}

void RFMock::set_frequency(int64_t freq_hz) { frequency_ = freq_hz; }
int64_t RFMock::frequency() const { return frequency_; }

void RFMock::set_sample_rate(uint32_t rate) { sample_rate_ = rate; }
uint32_t RFMock::sample_rate() const { return sample_rate_; }

void RFMock::set_bandwidth(uint32_t bw_hz) { bandwidth_ = bw_hz; }
uint32_t RFMock::bandwidth() const { return bandwidth_; }

void RFMock::set_lna_gain(int32_t gain_db) { lna_gain_ = gain_db; }
int32_t RFMock::lna_gain() const { return lna_gain_; }

void RFMock::set_vga_gain(int32_t gain_db) { vga_gain_ = gain_db; }
int32_t RFMock::vga_gain() const { return vga_gain_; }

void RFMock::set_tx_gain(int32_t gain_db) { tx_gain_ = gain_db; }
int32_t RFMock::tx_gain() const { return tx_gain_; }

void RFMock::set_amp_enable(bool enable) { amp_enabled_ = enable; }
bool RFMock::amp_enabled() const { return amp_enabled_; }

void RFMock::set_antenna_bias(bool enable) { antenna_bias_ = enable; }
bool RFMock::antenna_bias() const { return antenna_bias_; }

void RFMock::set_direction(Direction dir) { direction_ = dir; }
IRF::Direction RFMock::direction() const { return direction_; }

void RFMock::start_rx() { streaming_ = true; }
void RFMock::start_tx() { streaming_ = true; }
void RFMock::stop() { streaming_ = false; }
bool RFMock::is_streaming() const { return streaming_; }

void RFMock::set_rx_callback(RxCallback callback) { rx_callback_ = callback; }
void RFMock::set_tx_callback(TxCallback callback) { tx_callback_ = callback; }

void RFMock::load_iq_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open IQ file: " << path << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    iq_data_.resize(size);
    if (!file.read(reinterpret_cast<char*>(iq_data_.data()), size)) {
        std::cerr << "Failed to read IQ file" << std::endl;
        iq_data_.clear();
        return;
    }

    iq_position_ = 0;
    std::cout << "Loaded " << size << " bytes of IQ data" << std::endl;
}

int RFMock::read_samples(int8_t* buffer, size_t count) {
    if (!streaming_) return 0;

    // If we have IQ data loaded, play it back
    if (!iq_data_.empty()) {
        size_t bytes_needed = count * 2;  // IQ pairs
        size_t bytes_copied = 0;

        while (bytes_copied < bytes_needed) {
            size_t available = iq_data_.size() - iq_position_;
            size_t to_copy = std::min(available, bytes_needed - bytes_copied);

            std::memcpy(buffer + bytes_copied, iq_data_.data() + iq_position_, to_copy);
            bytes_copied += to_copy;
            iq_position_ += to_copy;

            if (iq_position_ >= iq_data_.size()) {
                if (loop_iq_) {
                    iq_position_ = 0;
                } else {
                    break;
                }
            }
        }

        return bytes_copied / 2;
    }

    // Otherwise generate a test signal
    generate_samples(buffer, count);
    return count;
}

int RFMock::write_samples(const int8_t* buffer, size_t count) {
    (void)buffer;
    return count;  // Pretend we transmitted everything
}

void RFMock::generate_samples(int8_t* buffer, size_t count) {
    double phase_inc = 2.0 * M_PI * test_freq_ / sample_rate_;

    for (size_t i = 0; i < count; ++i) {
        // Generate complex sinusoid + some noise
        double noise_i = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;
        double noise_q = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;

        double val_i = test_amplitude_ * std::cos(phase_) + noise_i;
        double val_q = test_amplitude_ * std::sin(phase_) + noise_q;

        // Convert to signed 8-bit
        buffer[i * 2 + 0] = static_cast<int8_t>(std::clamp(val_i * 127.0, -128.0, 127.0));
        buffer[i * 2 + 1] = static_cast<int8_t>(std::clamp(val_q * 127.0, -128.0, 127.0));

        phase_ += phase_inc;
        if (phase_ >= 2.0 * M_PI) {
            phase_ -= 2.0 * M_PI;
        }
    }
}

} // namespace hal
