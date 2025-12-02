/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "event_pc.hpp"
#include "portapack_shim.hpp"
#include "display_shim.hpp"
#include <thread>
#include <iostream>

// Static member initialization
std::atomic<uint32_t> EventDispatcher::pending_events_{0};
std::mutex EventDispatcher::event_mutex_;
std::condition_variable EventDispatcher::event_cv_;
ui::Point EventDispatcher::last_touch_point_{0, 0};
bool EventDispatcher::touch_active_{false};
std::queue<ui::KeyEvent> EventDispatcher::key_queue_;
std::mutex EventDispatcher::key_mutex_;
std::atomic<int32_t> EventDispatcher::encoder_delta_{0};

EventDispatcher::EventDispatcher(ui::Widget* top_widget, ui::Context& context)
    : top_widget_(top_widget)
    , context_(context)
    , last_frame_time_(std::chrono::steady_clock::now())
{
}

EventDispatcher::~EventDispatcher() {
    request_stop();
}

void EventDispatcher::run() {
    is_running_ = true;

    // Set up input callbacks from SDL
    portapack_shim::input_hal.set_key_callback([](hal::InputSDL::KeyEvent key, bool pressed) {
        if (pressed) {
            inject_key(static_cast<ui::KeyEvent>(key));
            events_flag(EVT_MASK_SWITCHES);
        }
    });

    portapack_shim::input_hal.set_encoder_callback([](int32_t delta) {
        encoder_delta_ += delta;
        events_flag(EVT_MASK_ENCODER);
    });

    portapack_shim::input_hal.set_touch_callback([](const hal::InputSDL::TouchEvent& event) {
        last_touch_point_ = {event.x, event.y};
        touch_active_ = (event.type != hal::InputSDL::TouchType::End);
        events_flag(EVT_MASK_TOUCH);
    });

    while (is_running_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - last_frame_time_;

        // Poll SDL events
        portapack_shim::input_hal.poll();

        // Check for quit request
        if (portapack_shim::input_hal.quit_requested()) {
            request_stop();
            break;
        }

        // Process pending events
        uint32_t events = pending_events_.exchange(0);
        if (events) {
            dispatch(events);
        }

        // Frame sync at ~60 FPS
        if (elapsed >= FRAME_DURATION) {
            last_frame_time_ = now;

            // Signal frame sync
            events_flag(EVT_MASK_LCD_FRAME_SYNC);

            // RTC tick approximately every second
            static int frame_count = 0;
            if (++frame_count >= FRAME_RATE) {
                frame_count = 0;
                events_flag(EVT_MASK_RTC_TICK);
            }
        }

        // Yield to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    is_running_ = false;
}

void EventDispatcher::request_stop() {
    is_running_ = false;
    event_cv_.notify_all();
}

void EventDispatcher::events_flag(uint32_t events) {
    pending_events_ |= events;
    event_cv_.notify_one();
}

void EventDispatcher::emulateTouch(ui::Point point) {
    last_touch_point_ = point;
    touch_active_ = true;
    events_flag(EVT_MASK_TOUCH);
}

void EventDispatcher::inject_key(ui::KeyEvent key) {
    std::lock_guard<std::mutex> lock(key_mutex_);
    key_queue_.push(key);
}

void EventDispatcher::inject_encoder(int32_t delta) {
    encoder_delta_ += delta;
    events_flag(EVT_MASK_ENCODER);
}

void EventDispatcher::wait_finish_frame() {
    // Wait for next frame sync
    std::unique_lock<std::mutex> lock(event_mutex_);
    event_cv_.wait_for(lock, FRAME_DURATION);
}

ui::Widget* EventDispatcher::getFocusedWidget() {
    // TODO: Implement focus management
    return top_widget_;
}

void EventDispatcher::dispatch(uint32_t events) {
    if (events & EVT_MASK_SWITCHES) {
        handle_switches();
    }

    if (events & EVT_MASK_ENCODER) {
        handle_encoder();
    }

    if (events & EVT_MASK_TOUCH) {
        handle_touch();
    }

    if (events & EVT_MASK_RTC_TICK) {
        handle_rtc_tick();
    }

    if (events & EVT_MASK_LCD_FRAME_SYNC) {
        handle_lcd_frame_sync();
    }
}

void EventDispatcher::handle_switches() {
    std::lock_guard<std::mutex> lock(key_mutex_);

    while (!key_queue_.empty()) {
        ui::KeyEvent key = key_queue_.front();
        key_queue_.pop();

        // Bubble the key event through widget hierarchy
        event_bubble_key(key);
    }
}

void EventDispatcher::handle_encoder() {
    int32_t delta = encoder_delta_.exchange(0);
    if (delta != 0) {
        event_bubble_encoder(delta);
    }
}

void EventDispatcher::handle_touch() {
    if (touch_active_) {
        ui::TouchEvent event;
        event.point = last_touch_point_;
        event.type = ui::TouchEvent::Type::Start;
        event_bubble_touch(event);
    }
}

void EventDispatcher::handle_rtc_tick() {
    // Handle RTC tick - update time display, etc.
}

void EventDispatcher::handle_lcd_frame_sync() {
    // Call tick callback for animation/updates
    if (tick_callback_) {
        tick_callback_();
    }

    // Present the frame
    shim::get_display().present();
}

bool EventDispatcher::event_bubble_key(ui::KeyEvent event) {
    // TODO: Implement full widget hierarchy bubbling
    // For now, just log the event
    std::cout << "Key event: " << static_cast<int>(event) << std::endl;

    // Try to handle in focused widget, then bubble up
    ui::Widget* widget = getFocusedWidget();
    if (widget) {
        // Would call widget->on_key(event) here
        // Return true if consumed
    }

    return false;
}

void EventDispatcher::event_bubble_encoder(ui::EncoderEvent event) {
    std::cout << "Encoder event: " << event << std::endl;

    ui::Widget* widget = getFocusedWidget();
    if (widget) {
        // Would call widget->on_encoder(event) here
    }
}

void EventDispatcher::event_bubble_touch(ui::TouchEvent event) {
    std::cout << "Touch event at (" << event.point.x << ", " << event.point.y << ")" << std::endl;

    // Find widget at touch point and dispatch
    if (top_widget_) {
        // Would call top_widget_->on_touch(event) here
    }
}

#endif // PORTAPACK_PC_EMULATOR
