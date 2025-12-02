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

#ifndef __HAL_RF_HPP__
#define __HAL_RF_HPP__

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>

namespace hal {

/**
 * @brief Abstract interface for RF hardware abstraction layer.
 *
 * This interface defines the contract that all RF implementations
 * must fulfill, allowing the application to work with different RF
 * backends (HackRF via MAX2837/RFFC5072, SoapySDR, RTL-SDR, etc.)
 */
class IRF {
public:
    // Direction for RF operation
    enum class Direction {
        Receive = 0,
        Transmit = 1
    };

    // Callback for receiving IQ samples
    using RxCallback = std::function<void(const int8_t* samples, size_t count)>;

    // Callback for transmitting IQ samples (return number of samples filled)
    using TxCallback = std::function<size_t(int8_t* buffer, size_t max_count)>;

    virtual ~IRF() = default;

    /**
     * @brief Initialize the RF hardware
     * @param device_args Optional device-specific arguments
     * @return true on success, false on failure
     */
    virtual bool init(const std::string& device_args = "") = 0;

    /**
     * @brief Shutdown the RF hardware
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if RF hardware is available and initialized
     * @return true if hardware is available
     */
    virtual bool is_available() const = 0;

    /**
     * @brief Get device information string
     * @return Human-readable device description
     */
    virtual std::string device_info() const = 0;

    // Frequency control

    /**
     * @brief Set the center frequency
     * @param freq_hz Frequency in Hz
     */
    virtual void set_frequency(int64_t freq_hz) = 0;

    /**
     * @brief Get the current center frequency
     * @return Frequency in Hz
     */
    virtual int64_t frequency() const = 0;

    // Sample rate control

    /**
     * @brief Set the sample rate
     * @param rate Sample rate in Hz
     */
    virtual void set_sample_rate(uint32_t rate) = 0;

    /**
     * @brief Get the current sample rate
     * @return Sample rate in Hz
     */
    virtual uint32_t sample_rate() const = 0;

    // Bandwidth control

    /**
     * @brief Set the baseband filter bandwidth
     * @param bw_hz Bandwidth in Hz
     */
    virtual void set_bandwidth(uint32_t bw_hz) = 0;

    /**
     * @brief Get the current bandwidth
     * @return Bandwidth in Hz
     */
    virtual uint32_t bandwidth() const = 0;

    // Gain control

    /**
     * @brief Set LNA (RF front-end) gain
     * @param gain_db Gain in dB (typically 0-40)
     */
    virtual void set_lna_gain(int32_t gain_db) = 0;

    /**
     * @brief Get LNA gain
     * @return Gain in dB
     */
    virtual int32_t lna_gain() const = 0;

    /**
     * @brief Set VGA (IF) gain
     * @param gain_db Gain in dB (typically 0-62)
     */
    virtual void set_vga_gain(int32_t gain_db) = 0;

    /**
     * @brief Get VGA gain
     * @return Gain in dB
     */
    virtual int32_t vga_gain() const = 0;

    /**
     * @brief Set TX gain
     * @param gain_db Gain in dB (typically 0-47)
     */
    virtual void set_tx_gain(int32_t gain_db) = 0;

    /**
     * @brief Get TX gain
     * @return Gain in dB
     */
    virtual int32_t tx_gain() const = 0;

    /**
     * @brief Enable/disable RF amplifier
     * @param enable true to enable amplifier
     */
    virtual void set_amp_enable(bool enable) = 0;

    /**
     * @brief Check if RF amplifier is enabled
     * @return true if amplifier is enabled
     */
    virtual bool amp_enabled() const = 0;

    // Antenna bias

    /**
     * @brief Enable/disable antenna bias voltage
     * @param enable true to enable bias
     */
    virtual void set_antenna_bias(bool enable) = 0;

    /**
     * @brief Check if antenna bias is enabled
     * @return true if bias is enabled
     */
    virtual bool antenna_bias() const = 0;

    // Streaming control

    /**
     * @brief Set the RF direction
     * @param dir Receive or Transmit
     */
    virtual void set_direction(Direction dir) = 0;

    /**
     * @brief Get the current RF direction
     * @return Current direction
     */
    virtual Direction direction() const = 0;

    /**
     * @brief Start RX streaming
     */
    virtual void start_rx() = 0;

    /**
     * @brief Start TX streaming
     */
    virtual void start_tx() = 0;

    /**
     * @brief Stop all streaming
     */
    virtual void stop() = 0;

    /**
     * @brief Check if streaming is active
     * @return true if streaming
     */
    virtual bool is_streaming() const = 0;

    // Sample I/O

    /**
     * @brief Read IQ samples from RX buffer
     * @param buffer Buffer to store samples (I,Q pairs as int8_t)
     * @param count Number of IQ sample pairs to read
     * @return Number of sample pairs actually read
     */
    virtual int read_samples(int8_t* buffer, size_t count) = 0;

    /**
     * @brief Write IQ samples to TX buffer
     * @param buffer Buffer containing samples (I,Q pairs as int8_t)
     * @param count Number of IQ sample pairs to write
     * @return Number of sample pairs actually written
     */
    virtual int write_samples(const int8_t* buffer, size_t count) = 0;

    /**
     * @brief Set callback for RX samples
     * @param callback Function to call when samples are available
     */
    virtual void set_rx_callback(RxCallback callback) = 0;

    /**
     * @brief Set callback for TX samples
     * @param callback Function to call when samples are needed
     */
    virtual void set_tx_callback(TxCallback callback) = 0;
};

} // namespace hal

#endif // __HAL_RF_HPP__
