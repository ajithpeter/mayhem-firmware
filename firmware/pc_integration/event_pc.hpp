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

#ifndef __EVENT_PC_HPP__
#define __EVENT_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

// UI types
namespace ui {
    struct Point {
        int x, y;
        constexpr int x_() const { return x; }
        constexpr int y_() const { return y; }
    };

    struct Context;
    class Widget;

    enum class KeyEvent : uint8_t {
        Right = 0,
        Left = 1,
        Down = 2,
        Up = 3,
        Select = 4,
        Dfu = 5,
        Back = 6
    };

    using EncoderEvent = int32_t;

    struct TouchEvent {
        enum class Type { Start, Move, End };
        Point point;
        Type type;
    };
}

// Event masks matching original firmware
#define EVT_MASK_RTC_TICK       (1 << 0)
#define EVT_MASK_LCD_FRAME_SYNC (1 << 1)
#define EVT_MASK_SWITCHES       (1 << 2)
#define EVT_MASK_ENCODER        (1 << 3)
#define EVT_MASK_TOUCH          (1 << 4)
#define EVT_MASK_APPLICATION    (1 << 5)
#define EVT_MASK_LOCAL          (1 << 6)
#define EVT_MASK_USB            (1 << 7)

/**
 * @brief PC-compatible EventDispatcher for PortaPack UI.
 *
 * This class provides the same interface as the ChibiOS-based
 * EventDispatcher but uses SDL2 and standard C++ threading.
 */
class EventDispatcher {
public:
    using callback_t = std::function<void()>;

    EventDispatcher(ui::Widget* top_widget, ui::Context& context);
    ~EventDispatcher();

    // Main event loop
    void run();
    void request_stop();
    bool is_running() const { return is_running_; }

    // Event signaling (replaces ChibiOS chEvtSignal)
    static void events_flag(uint32_t events);

    // Input event injection (for PC emulator)
    static void emulateTouch(ui::Point point);
    static void inject_key(ui::KeyEvent key);
    static void inject_encoder(int32_t delta);

    // Frame synchronization
    void wait_finish_frame();

    // Widget access
    ui::Widget* getTopWidget() const { return top_widget_; }
    ui::Widget* getFocusedWidget();

    // Set tick callback for frame updates
    void set_tick_callback(callback_t callback) { tick_callback_ = callback; }

private:
    void dispatch(uint32_t events);
    void handle_switches();
    void handle_encoder();
    void handle_touch();
    void handle_rtc_tick();
    void handle_lcd_frame_sync();

    bool event_bubble_key(ui::KeyEvent event);
    void event_bubble_encoder(ui::EncoderEvent event);
    void event_bubble_touch(ui::TouchEvent event);

    ui::Widget* top_widget_;
    ui::Context& context_;
    std::atomic<bool> is_running_{false};
    callback_t tick_callback_;

    // Event state
    static std::atomic<uint32_t> pending_events_;
    static std::mutex event_mutex_;
    static std::condition_variable event_cv_;

    // Touch state
    static ui::Point last_touch_point_;
    static bool touch_active_;

    // Key state
    static std::queue<ui::KeyEvent> key_queue_;
    static std::mutex key_mutex_;

    // Encoder state
    static std::atomic<int32_t> encoder_delta_;

    // Timing
    std::chrono::steady_clock::time_point last_frame_time_;
    static constexpr int FRAME_RATE = 60;
    static constexpr auto FRAME_DURATION = std::chrono::milliseconds(1000 / FRAME_RATE);
};

#endif // PORTAPACK_PC_EMULATOR

#endif // __EVENT_PC_HPP__
