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

/**
 * @file test_shim_event.cpp
 * @brief Unit tests for event shim.
 */

#include "doctest.h"
#include "event_shim.hpp"
#include <functional>
#include <set>

TEST_SUITE_BEGIN("EventShim");

// ============================================================================
// KeyEvent Tests
// ============================================================================

TEST_CASE("KeyEvent enumeration") {
    SUBCASE("All key events defined") {
        CHECK(static_cast<int>(shim::KeyEvent::Right) == 0);
        CHECK(static_cast<int>(shim::KeyEvent::Left) == 1);
        CHECK(static_cast<int>(shim::KeyEvent::Down) == 2);
        CHECK(static_cast<int>(shim::KeyEvent::Up) == 3);
        CHECK(static_cast<int>(shim::KeyEvent::Select) == 4);
        CHECK(static_cast<int>(shim::KeyEvent::Dfu) == 5);
        CHECK(static_cast<int>(shim::KeyEvent::Back) == 6);
    }

    SUBCASE("Key events are unique") {
        std::set<int> values;
        values.insert(static_cast<int>(shim::KeyEvent::Right));
        values.insert(static_cast<int>(shim::KeyEvent::Left));
        values.insert(static_cast<int>(shim::KeyEvent::Down));
        values.insert(static_cast<int>(shim::KeyEvent::Up));
        values.insert(static_cast<int>(shim::KeyEvent::Select));
        values.insert(static_cast<int>(shim::KeyEvent::Dfu));
        values.insert(static_cast<int>(shim::KeyEvent::Back));

        CHECK_EQ(values.size(), 7);
    }
}

// ============================================================================
// TouchEvent Tests
// ============================================================================

TEST_CASE("TouchEvent structure") {
    SUBCASE("Touch types") {
        CHECK(static_cast<int>(shim::TouchEvent::Type::Start) == 0);
        CHECK(static_cast<int>(shim::TouchEvent::Type::Move) == 1);
        CHECK(static_cast<int>(shim::TouchEvent::Type::End) == 2);
    }

    SUBCASE("Create touch event") {
        shim::TouchEvent event;
        event.x = 120;
        event.y = 160;
        event.type = shim::TouchEvent::Type::Start;

        CHECK_EQ(event.x, 120);
        CHECK_EQ(event.y, 160);
        CHECK(event.type == shim::TouchEvent::Type::Start);
    }
}

// ============================================================================
// EventDispatcher Constants Tests
// ============================================================================

TEST_CASE("EventDispatcher constants") {
    SUBCASE("Event masks are power of 2") {
        CHECK_EQ(shim::EventDispatcher::EVT_MASK_SWITCHES, 1 << 0);
        CHECK_EQ(shim::EventDispatcher::EVT_MASK_ENCODER, 1 << 1);
        CHECK_EQ(shim::EventDispatcher::EVT_MASK_TOUCH, 1 << 2);
        CHECK_EQ(shim::EventDispatcher::EVT_MASK_RTC_TICK, 1 << 3);
        CHECK_EQ(shim::EventDispatcher::EVT_MASK_APPLICATION, 1 << 4);
    }

    SUBCASE("Event masks are unique") {
        uint32_t all_masks =
            shim::EventDispatcher::EVT_MASK_SWITCHES |
            shim::EventDispatcher::EVT_MASK_ENCODER |
            shim::EventDispatcher::EVT_MASK_TOUCH |
            shim::EventDispatcher::EVT_MASK_RTC_TICK |
            shim::EventDispatcher::EVT_MASK_APPLICATION;

        // If all unique and power of 2, should equal sum
        CHECK_EQ(all_masks, 0b11111);
    }

    SUBCASE("Event mask combinations") {
        uint32_t input_events =
            shim::EventDispatcher::EVT_MASK_SWITCHES |
            shim::EventDispatcher::EVT_MASK_ENCODER |
            shim::EventDispatcher::EVT_MASK_TOUCH;

        CHECK((input_events & shim::EventDispatcher::EVT_MASK_SWITCHES) != 0);
        CHECK((input_events & shim::EventDispatcher::EVT_MASK_ENCODER) != 0);
        CHECK((input_events & shim::EventDispatcher::EVT_MASK_TOUCH) != 0);
        CHECK((input_events & shim::EventDispatcher::EVT_MASK_RTC_TICK) == 0);
    }
}

// ============================================================================
// EventDispatcher Creation Tests
// ============================================================================

TEST_CASE("EventDispatcher creation") {
    SUBCASE("Create dispatcher") {
        shim::EventDispatcher dispatcher;
        CHECK_FALSE(dispatcher.should_stop());
    }

    SUBCASE("Request stop") {
        shim::EventDispatcher dispatcher;
        CHECK_FALSE(dispatcher.should_stop());
        dispatcher.request_stop();
        CHECK(dispatcher.should_stop());
    }
}

// ============================================================================
// Callback Registration Tests
// ============================================================================

TEST_CASE("Callback registration") {
    shim::EventDispatcher dispatcher;

    SUBCASE("Key callback") {
        bool callback_called = false;
        shim::KeyEvent received_key;
        bool received_pressed;

        dispatcher.set_key_callback([&](shim::KeyEvent key, bool pressed) {
            callback_called = true;
            received_key = key;
            received_pressed = pressed;
        });

        // Callback is set but not called yet
        CHECK_FALSE(callback_called);
    }

    SUBCASE("Encoder callback") {
        bool callback_called = false;
        int32_t received_delta = 0;

        dispatcher.set_encoder_callback([&](int32_t delta) {
            callback_called = true;
            received_delta = delta;
        });

        CHECK_FALSE(callback_called);
    }

    SUBCASE("Touch callback") {
        bool callback_called = false;

        dispatcher.set_touch_callback([&](const shim::TouchEvent& event) {
            callback_called = true;
        });

        CHECK_FALSE(callback_called);
    }

    SUBCASE("Tick callback") {
        bool callback_called = false;

        dispatcher.set_tick_callback([&]() {
            callback_called = true;
        });

        CHECK_FALSE(callback_called);
    }

    SUBCASE("Idle callback") {
        bool callback_called = false;

        dispatcher.set_idle_callback([&]() {
            callback_called = true;
        });

        CHECK_FALSE(callback_called);
    }
}

// ============================================================================
// Encoder Position Tests
// ============================================================================

TEST_CASE("Encoder position") {
    shim::EventDispatcher dispatcher;

    SUBCASE("Initial position is zero") {
        CHECK_EQ(dispatcher.encoder_position(), 0);
    }
}

// ============================================================================
// Touch State Tests
// ============================================================================

TEST_CASE("Touch state") {
    shim::EventDispatcher dispatcher;

    SUBCASE("No active touch initially") {
        int16_t x, y;
        CHECK_FALSE(dispatcher.get_touch(x, y));
    }
}

// ============================================================================
// Event Flag Tests
// ============================================================================

TEST_CASE("Event flags") {
    SUBCASE("Set event flags") {
        // Static method test
        shim::EventDispatcher::events_flag(shim::EventDispatcher::EVT_MASK_SWITCHES);
        CHECK(true);  // Just verify no crash

        shim::EventDispatcher::events_flag(
            shim::EventDispatcher::EVT_MASK_ENCODER |
            shim::EventDispatcher::EVT_MASK_TOUCH
        );
        CHECK(true);
    }
}

// ============================================================================
// Global Dispatcher Tests
// ============================================================================

TEST_CASE("Global event dispatcher") {
    SUBCASE("Get global instance") {
        auto& dispatcher = shim::get_event_dispatcher();
        CHECK(&dispatcher != nullptr);
    }

    SUBCASE("Same instance returned") {
        auto& dispatcher1 = shim::get_event_dispatcher();
        auto& dispatcher2 = shim::get_event_dispatcher();
        CHECK(&dispatcher1 == &dispatcher2);
    }
}

// ============================================================================
// Callback Type Tests
// ============================================================================

TEST_CASE("Callback types") {
    SUBCASE("KeyCallback type") {
        shim::EventDispatcher::KeyCallback callback =
            [](shim::KeyEvent key, bool pressed) {};
        CHECK(callback != nullptr);
    }

    SUBCASE("EncoderCallback type") {
        shim::EventDispatcher::EncoderCallback callback =
            [](int32_t delta) {};
        CHECK(callback != nullptr);
    }

    SUBCASE("TouchCallback type") {
        shim::EventDispatcher::TouchCallback callback =
            [](const shim::TouchEvent& event) {};
        CHECK(callback != nullptr);
    }

    SUBCASE("TickCallback type") {
        shim::EventDispatcher::TickCallback callback = []() {};
        CHECK(callback != nullptr);
    }

    SUBCASE("IdleCallback type") {
        shim::EventDispatcher::IdleCallback callback = []() {};
        CHECK(callback != nullptr);
    }
}

// ============================================================================
// Key Press State Tests
// ============================================================================

TEST_CASE("Key press state") {
    shim::EventDispatcher dispatcher;

    SUBCASE("No keys pressed initially") {
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Right));
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Left));
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Up));
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Down));
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Select));
        CHECK_FALSE(dispatcher.is_key_pressed(shim::KeyEvent::Back));
    }

    SUBCASE("Long press detection initially false") {
        CHECK_FALSE(dispatcher.is_key_long_pressed(shim::KeyEvent::Select));
    }
}

TEST_SUITE_END();
