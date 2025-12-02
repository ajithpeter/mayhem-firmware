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

#ifndef __TIME_SYSTEM_HPP__
#define __TIME_SYSTEM_HPP__

#include "../interface/hal_time.hpp"
#include <chrono>
#include <thread>
#include <atomic>

namespace hal {

/**
 * @brief System time-based implementation for PC emulation.
 *
 * This class implements the ITime interface using the C++ standard
 * library chrono facilities, providing RTC emulation on PC.
 */
class TimeSystem : public ITime {
public:
    TimeSystem();
    ~TimeSystem() override;

    // ITime interface implementation
    bool init() override;
    void shutdown() override;
    DateTime now() const override;
    void set_datetime(const DateTime& dt) override;
    uint32_t unix_time() const override;
    void set_unix_time(uint32_t timestamp) override;
    uint64_t millis() const override;
    uint64_t micros() const override;
    void delay_ms(uint32_t ms) override;
    void delay_us(uint32_t us) override;

private:
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<int64_t> time_offset_{0};  // Offset from real time in seconds
    std::atomic<bool> initialized_{false};
};

} // namespace hal

#endif // __TIME_SYSTEM_HPP__
