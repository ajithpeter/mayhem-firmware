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

#ifndef __SHARED_MEMORY_SHIM_HPP__
#define __SHARED_MEMORY_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <cstddef>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

namespace shim {

/**
 * @brief Thread-safe message queue for PC emulation.
 *
 * This replaces the hardware-based message queues used for
 * inter-processor communication on the LPC43xx.
 *
 * @tparam T Message type
 * @tparam N Maximum queue size
 */
template<typename T, size_t N>
class ThreadSafeMessageQueue {
public:
    /**
     * @brief Push a message onto the queue.
     * @param message Message to push
     * @return true if successful, false if queue is full
     */
    bool push(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= N) {
            return false;
        }
        queue_.push(message);
        cv_.notify_one();
        return true;
    }

    /**
     * @brief Pop a message from the queue.
     * @param message Output message
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     * @return true if a message was retrieved, false on timeout/empty
     */
    bool pop(T& message, uint32_t timeout_ms = 0) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeout_ms > 0) {
            if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              [this] { return !queue_.empty(); })) {
                return false;
            }
        } else if (queue_.empty()) {
            return false;
        }

        message = queue_.front();
        queue_.pop();
        return true;
    }

    /**
     * @brief Check if the queue is empty.
     * @return true if empty
     */
    bool is_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Get number of messages in queue.
     * @return Number of messages
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Clear all messages from the queue.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

/**
 * @brief Channel spectrum data structure.
 */
struct ChannelSpectrum {
    std::array<uint8_t, 256> db{};
    uint32_t sampling_rate = 0;
    int32_t channel_filter_low_frequency = 0;
    int32_t channel_filter_high_frequency = 0;
    int32_t channel_filter_transition = 0;
};

/**
 * @brief Jammer channel configuration.
 */
struct JammerChannel {
    bool enabled = false;
    int64_t center_freq = 0;
    int64_t bandwidth = 0;
    uint32_t duration = 0;
};

/**
 * @brief Hopper channel configuration.
 */
struct HopperChannel {
    int64_t frequency = 0;
    uint32_t duration = 0;
};

/**
 * @brief Tone data for audio generation.
 */
struct ToneData {
    uint32_t frequency = 0;
    uint32_t duration = 0;
    uint32_t delta = 0;
};

/**
 * @brief Generic message structure for IPC.
 */
struct Message {
    uint32_t id;
    uint8_t data[508];

    Message() : id(0) {
        std::fill(std::begin(data), std::end(data), 0);
    }
};

/**
 * @brief PC implementation of SharedMemory.
 *
 * This structure emulates the shared memory region used for
 * inter-processor communication on the LPC43xx hardware.
 * On PC, we use thread-safe queues and atomic variables.
 */
struct SharedMemoryPC {
    // Message queues (M0 <-> M4 communication)
    ThreadSafeMessageQueue<Message, 16> baseband_queue;      // M0 → M4
    ThreadSafeMessageQueue<Message, 16> application_queue;   // M4 → M0

    // TX data buffer (for OOK, FSK, etc.)
    std::array<uint8_t, 32768> bb_data{};

    // M4 state flags
    std::atomic<uint32_t> m4_state{0};
    std::atomic<uint32_t> application_counter{0};
    std::atomic<uint32_t> baseband_counter{0};

    // Spectrum data
    ChannelSpectrum spectrum;
    std::mutex spectrum_mutex;

    // Jammer configuration
    std::array<JammerChannel, 80> jammer_channels{};

    // Frequency hopper configuration
    std::array<HopperChannel, 24> hopper_channels{};

    // Tone data
    ToneData tones_data;

    // AFSK/FSK data
    std::array<uint8_t, 512> modem_data{};

    // Baseband ready flag
    std::atomic<bool> baseband_ready{false};

    // Performance counters
    std::atomic<uint8_t> m4_performance_counter{0};
    std::atomic<uint16_t> m4_stack_usage{0};
    std::atomic<uint32_t> m4_heap_usage{0};
};

/**
 * @brief Get the shared memory instance.
 * @return Reference to the shared memory structure
 */
SharedMemoryPC& get_shared_memory();

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __SHARED_MEMORY_SHIM_HPP__
