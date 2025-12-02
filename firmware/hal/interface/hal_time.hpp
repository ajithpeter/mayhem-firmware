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

#ifndef __HAL_TIME_HPP__
#define __HAL_TIME_HPP__

#include <cstdint>

namespace hal {

/**
 * @brief Abstract interface for time/clock hardware abstraction layer.
 *
 * This interface defines the contract that all time implementations
 * must fulfill, allowing the application to work with different time
 * backends (LPC43xx RTC, system time, etc.)
 */
class ITime {
public:
    // Date/time structure
    struct DateTime {
        uint16_t year;    // Full year (e.g., 2024)
        uint8_t month;    // 1-12
        uint8_t day;      // 1-31
        uint8_t hour;     // 0-23
        uint8_t minute;   // 0-59
        uint8_t second;   // 0-59
        uint8_t weekday;  // 0-6 (Sunday = 0)
    };

    virtual ~ITime() = default;

    /**
     * @brief Initialize the time system
     * @return true on success, false on failure
     */
    virtual bool init() = 0;

    /**
     * @brief Shutdown the time system
     */
    virtual void shutdown() = 0;

    /**
     * @brief Get current date and time
     * @return DateTime structure
     */
    virtual DateTime now() const = 0;

    /**
     * @brief Set current date and time
     * @param dt DateTime structure
     */
    virtual void set_datetime(const DateTime& dt) = 0;

    /**
     * @brief Get Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
     * @return Unix timestamp
     */
    virtual uint32_t unix_time() const = 0;

    /**
     * @brief Set time from Unix timestamp
     * @param timestamp Unix timestamp
     */
    virtual void set_unix_time(uint32_t timestamp) = 0;

    /**
     * @brief Get milliseconds since system start
     * @return Milliseconds counter
     */
    virtual uint64_t millis() const = 0;

    /**
     * @brief Get microseconds since system start
     * @return Microseconds counter
     */
    virtual uint64_t micros() const = 0;

    /**
     * @brief Delay for specified milliseconds
     * @param ms Milliseconds to delay
     */
    virtual void delay_ms(uint32_t ms) = 0;

    /**
     * @brief Delay for specified microseconds
     * @param us Microseconds to delay
     */
    virtual void delay_us(uint32_t us) = 0;
};

} // namespace hal

#endif // __HAL_TIME_HPP__
