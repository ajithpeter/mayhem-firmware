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

#ifndef __EVENT_SHIM_HPP__
#define __EVENT_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <functional>

namespace shim {

/**
 * @brief Key event enumeration matching ui::KeyEvent
 */
enum class KeyEvent : uint8_t {
    Right = 0,
    Left = 1,
    Down = 2,
    Up = 3,
    Select = 4,
    Dfu = 5,
    Back = 6,
};

/**
 * @brief Touch event structure
 */
struct TouchEvent {
    enum class Type : uint32_t {
        Start = 0,
        Move = 1,
        End = 2,
    };

    int16_t x;
    int16_t y;
    Type type;
};

/**
 * @brief Event dispatcher shim for PC emulation.
 *
 * This replaces the ChibiOS-based event system with an SDL-based
 * implementation that polls for input events and dispatches them
 * to registered handlers.
 */
class EventDispatcher {
public:
    // Event masks
    static constexpr uint32_t EVT_MASK_SWITCHES = 1 << 0;
    static constexpr uint32_t EVT_MASK_ENCODER = 1 << 1;
    static constexpr uint32_t EVT_MASK_TOUCH = 1 << 2;
    static constexpr uint32_t EVT_MASK_RTC_TICK = 1 << 3;
    static constexpr uint32_t EVT_MASK_APPLICATION = 1 << 4;

    // Callback types
    using KeyCallback = std::function<void(KeyEvent key, bool pressed)>;
    using EncoderCallback = std::function<void(int32_t delta)>;
    using TouchCallback = std::function<void(const TouchEvent& event)>;
    using TickCallback = std::function<void()>;
    using IdleCallback = std::function<void()>;

    EventDispatcher();
    ~EventDispatcher();

    /**
     * @brief Run the main event loop.
     * This blocks until request_stop() is called.
     */
    void run();

    /**
     * @brief Request the event loop to stop.
     */
    void request_stop();

    /**
     * @brief Check if stop has been requested.
     * @return true if stop requested
     */
    bool should_stop() const { return should_stop_; }

    /**
     * @brief Set flags to trigger event processing.
     * @param flags Event mask flags
     */
    static void events_flag(uint32_t flags);

    /**
     * @brief Set callback for key events.
     */
    void set_key_callback(KeyCallback callback) { key_callback_ = callback; }

    /**
     * @brief Set callback for encoder events.
     */
    void set_encoder_callback(EncoderCallback callback) { encoder_callback_ = callback; }

    /**
     * @brief Set callback for touch events.
     */
    void set_touch_callback(TouchCallback callback) { touch_callback_ = callback; }

    /**
     * @brief Set callback for RTC tick events (called ~60 times/sec).
     */
    void set_tick_callback(TickCallback callback) { tick_callback_ = callback; }

    /**
     * @brief Set callback for idle processing.
     */
    void set_idle_callback(IdleCallback callback) { idle_callback_ = callback; }

    /**
     * @brief Get the current encoder position.
     */
    int32_t encoder_position() const { return encoder_position_; }

    /**
     * @brief Get the last touch position.
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @return true if touch is active
     */
    bool get_touch(int16_t& x, int16_t& y) const;

    /**
     * @brief Check if a key is currently pressed.
     */
    bool is_key_pressed(KeyEvent key) const;

    /**
     * @brief Check if a key is long-pressed.
     */
    bool is_key_long_pressed(KeyEvent key) const;

private:
    void poll_events();
    void process_key(KeyEvent key, bool pressed);
    void process_encoder(int32_t delta);
    void process_touch(int16_t x, int16_t y, TouchEvent::Type type);

    KeyCallback key_callback_;
    EncoderCallback encoder_callback_;
    TouchCallback touch_callback_;
    TickCallback tick_callback_;
    IdleCallback idle_callback_;

    int32_t encoder_position_ = 0;
    int16_t touch_x_ = 0;
    int16_t touch_y_ = 0;
    bool touching_ = false;

    bool should_stop_ = false;

    static uint32_t pending_flags_;
};

/**
 * @brief Get the global event dispatcher instance.
 */
EventDispatcher& get_event_dispatcher();

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __EVENT_SHIM_HPP__
