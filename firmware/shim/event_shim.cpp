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

#include "event_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include "display_shim.hpp"
#include <chrono>
#include <thread>

namespace shim {

static EventDispatcher event_dispatcher_instance;
uint32_t EventDispatcher::pending_flags_ = 0;

EventDispatcher& get_event_dispatcher() {
    return event_dispatcher_instance;
}

EventDispatcher::EventDispatcher() = default;

EventDispatcher::~EventDispatcher() = default;

void EventDispatcher::run() {
    should_stop_ = false;

    auto last_tick = std::chrono::steady_clock::now();
    constexpr auto tick_interval = std::chrono::milliseconds(16);  // ~60 FPS

    // Set up input callbacks
    portapack_shim::input_hal.set_key_callback([this](hal::IInput::KeyEvent key, bool pressed) {
        process_key(static_cast<KeyEvent>(static_cast<uint8_t>(key)), pressed);
    });

    portapack_shim::input_hal.set_encoder_callback([this](int32_t delta) {
        process_encoder(delta);
    });

    portapack_shim::input_hal.set_touch_callback([this](const hal::IInput::TouchEvent& event) {
        TouchEvent::Type type;
        switch (event.type) {
            case hal::IInput::TouchType::Start: type = TouchEvent::Type::Start; break;
            case hal::IInput::TouchType::Move:  type = TouchEvent::Type::Move; break;
            case hal::IInput::TouchType::End:   type = TouchEvent::Type::End; break;
        }
        process_touch(event.x, event.y, type);
    });

    while (!should_stop_) {
        // Poll input events
        portapack_shim::input_hal.poll();

        // Check for quit request
        if (portapack_shim::input_hal.quit_requested()) {
            request_stop();
            break;
        }

        // Handle tick timing
        auto now = std::chrono::steady_clock::now();
        if (now - last_tick >= tick_interval) {
            last_tick = now;

            // Fire tick callback
            if (tick_callback_) {
                tick_callback_();
            }

            // Update display
            get_display().present();
        }

        // Fire idle callback
        if (idle_callback_) {
            idle_callback_();
        }

        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void EventDispatcher::request_stop() {
    should_stop_ = true;
}

void EventDispatcher::events_flag(uint32_t flags) {
    pending_flags_ |= flags;
}

void EventDispatcher::poll_events() {
    portapack_shim::input_hal.poll();
}

void EventDispatcher::process_key(KeyEvent key, bool pressed) {
    if (key_callback_) {
        key_callback_(key, pressed);
    }
    events_flag(EVT_MASK_SWITCHES);
}

void EventDispatcher::process_encoder(int32_t delta) {
    encoder_position_ += delta;
    if (encoder_callback_) {
        encoder_callback_(delta);
    }
    events_flag(EVT_MASK_ENCODER);
}

void EventDispatcher::process_touch(int16_t x, int16_t y, TouchEvent::Type type) {
    touch_x_ = x;
    touch_y_ = y;
    touching_ = (type != TouchEvent::Type::End);

    if (touch_callback_) {
        TouchEvent event;
        event.x = x;
        event.y = y;
        event.type = type;
        touch_callback_(event);
    }
    events_flag(EVT_MASK_TOUCH);
}

bool EventDispatcher::get_touch(int16_t& x, int16_t& y) const {
    if (!touching_) return false;
    x = touch_x_;
    y = touch_y_;
    return true;
}

bool EventDispatcher::is_key_pressed(KeyEvent key) const {
    return portapack_shim::input_hal.is_key_pressed(
        static_cast<hal::IInput::KeyEvent>(static_cast<uint8_t>(key))
    );
}

bool EventDispatcher::is_key_long_pressed(KeyEvent key) const {
    return portapack_shim::input_hal.is_key_long_pressed(
        static_cast<hal::IInput::KeyEvent>(static_cast<uint8_t>(key))
    );
}

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
