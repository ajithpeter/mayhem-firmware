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

#include "time_system.hpp"
#include <ctime>
#include <iostream>

namespace hal {

TimeSystem::TimeSystem() = default;

TimeSystem::~TimeSystem() {
    shutdown();
}

bool TimeSystem::init() {
    start_time_ = std::chrono::steady_clock::now();
    time_offset_ = 0;
    initialized_ = true;
    std::cout << "TimeSystem initialized" << std::endl;
    return true;
}

void TimeSystem::shutdown() {
    initialized_ = false;
}

ITime::DateTime TimeSystem::now() const {
    DateTime dt{};

    // Get current time with offset
    auto sys_time = std::chrono::system_clock::now();
    auto adjusted_time = sys_time + std::chrono::seconds(time_offset_.load());

    std::time_t time = std::chrono::system_clock::to_time_t(adjusted_time);
    std::tm* tm = std::localtime(&time);

    if (tm) {
        dt.year = static_cast<uint16_t>(tm->tm_year + 1900);
        dt.month = static_cast<uint8_t>(tm->tm_mon + 1);
        dt.day = static_cast<uint8_t>(tm->tm_mday);
        dt.hour = static_cast<uint8_t>(tm->tm_hour);
        dt.minute = static_cast<uint8_t>(tm->tm_min);
        dt.second = static_cast<uint8_t>(tm->tm_sec);
        dt.weekday = static_cast<uint8_t>(tm->tm_wday);
    }

    return dt;
}

void TimeSystem::set_datetime(const DateTime& dt) {
    std::tm tm{};
    tm.tm_year = dt.year - 1900;
    tm.tm_mon = dt.month - 1;
    tm.tm_mday = dt.day;
    tm.tm_hour = dt.hour;
    tm.tm_min = dt.minute;
    tm.tm_sec = dt.second;

    std::time_t target_time = std::mktime(&tm);
    std::time_t current_time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    time_offset_ = target_time - current_time;
}

uint32_t TimeSystem::unix_time() const {
    auto sys_time = std::chrono::system_clock::now();
    auto adjusted_time = sys_time + std::chrono::seconds(time_offset_.load());

    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            adjusted_time.time_since_epoch()
        ).count()
    );
}

void TimeSystem::set_unix_time(uint32_t timestamp) {
    std::time_t current_time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    time_offset_ = static_cast<int64_t>(timestamp) - current_time;
}

uint64_t TimeSystem::millis() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_
    ).count();
}

uint64_t TimeSystem::micros() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
        now - start_time_
    ).count();
}

void TimeSystem::delay_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void TimeSystem::delay_us(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

} // namespace hal
