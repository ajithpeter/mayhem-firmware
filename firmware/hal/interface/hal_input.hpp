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

#ifndef __HAL_INPUT_HPP__
#define __HAL_INPUT_HPP__

#include <cstdint>
#include <functional>

namespace hal {

/**
 * @brief Abstract interface for input hardware abstraction layer.
 *
 * This interface defines the contract that all input implementations
 * must fulfill, allowing the application to work with different input
 * backends (GPIO buttons/encoder, SDL keyboard/mouse, etc.)
 */
class IInput {
public:
    // Key events matching PortaPack buttons
    enum class KeyEvent : uint8_t {
        Right = 0,
        Left = 1,
        Down = 2,
        Up = 3,
        Select = 4,
        Dfu = 5,
        Back = 6,  // Left and Up together, or ESC key
    };

    // Touch event types
    enum class TouchType : uint32_t {
        Start = 0,
        Move = 1,
        End = 2,
    };

    // Touch event data
    struct TouchEvent {
        int16_t x;
        int16_t y;
        TouchType type;
    };

    // Callbacks for input events
    using KeyCallback = std::function<void(KeyEvent key, bool pressed)>;
    using EncoderCallback = std::function<void(int32_t delta)>;
    using TouchCallback = std::function<void(const TouchEvent& event)>;

    virtual ~IInput() = default;

    /**
     * @brief Initialize the input hardware
     * @return true on success, false on failure
     */
    virtual bool init() = 0;

    /**
     * @brief Shutdown the input hardware
     */
    virtual void shutdown() = 0;

    /**
     * @brief Poll for input events
     * Call this periodically to process pending input events.
     * Events will be delivered through registered callbacks.
     */
    virtual void poll() = 0;

    /**
     * @brief Check if a key is currently pressed
     * @param key The key to check
     * @return true if the key is currently pressed
     */
    virtual bool is_key_pressed(KeyEvent key) const = 0;

    /**
     * @brief Check if a key has been pressed long enough to trigger repeat
     * @param key The key to check
     * @return true if long-press threshold has been reached
     */
    virtual bool is_key_long_pressed(KeyEvent key) const = 0;

    /**
     * @brief Get current encoder position
     * @return Accumulated encoder position
     */
    virtual int32_t encoder_position() const = 0;

    /**
     * @brief Reset encoder position to zero
     */
    virtual void reset_encoder() = 0;

    /**
     * @brief Get the last touch position
     * @param x Output: X coordinate
     * @param y Output: Y coordinate
     * @return true if touch is currently active
     */
    virtual bool get_touch(int16_t& x, int16_t& y) const = 0;

    /**
     * @brief Check if touch is currently active
     * @return true if screen is being touched
     */
    virtual bool is_touching() const = 0;

    /**
     * @brief Set callback for key events
     * @param callback Function to call on key events
     */
    virtual void set_key_callback(KeyCallback callback) = 0;

    /**
     * @brief Set callback for encoder events
     * @param callback Function to call on encoder rotation
     */
    virtual void set_encoder_callback(EncoderCallback callback) = 0;

    /**
     * @brief Set callback for touch events
     * @param callback Function to call on touch events
     */
    virtual void set_touch_callback(TouchCallback callback) = 0;

    /**
     * @brief Check if quit was requested (e.g., window close)
     * @return true if application should exit
     */
    virtual bool quit_requested() const = 0;
};

} // namespace hal

#endif // __HAL_INPUT_HPP__
