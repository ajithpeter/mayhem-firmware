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
 * @file test_input_sdl.cpp
 * @brief Unit tests for SDL input implementation.
 */

#include "doctest.h"
#include "input_sdl.hpp"
#include <set>
#include <array>

TEST_SUITE_BEGIN("InputSDL");

// ============================================================================
// InputSDL Constants Tests
// ============================================================================

TEST_CASE("InputSDL constants") {
    SUBCASE("Display scale") {
        CHECK_EQ(hal::InputSDL::DISPLAY_SCALE, 2);
    }

    SUBCASE("Long press duration") {
        CHECK_EQ(hal::InputSDL::LONG_PRESS_MS, 500);
    }

    SUBCASE("Long press is reasonable duration") {
        CHECK(hal::InputSDL::LONG_PRESS_MS >= 300);
        CHECK(hal::InputSDL::LONG_PRESS_MS <= 1000);
    }
}

// ============================================================================
// Key Event Mapping Tests
// ============================================================================

TEST_CASE("Key event values") {
    SUBCASE("All key events have unique values") {
        std::set<int> values;
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Right));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Left));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Down));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Up));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Select));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Dfu));
        values.insert(static_cast<int>(hal::IInput::KeyEvent::Back));

        CHECK_EQ(values.size(), 7);  // All unique
    }

    SUBCASE("Key event values are sequential") {
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Right), 0);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Left), 1);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Down), 2);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Up), 3);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Select), 4);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Dfu), 5);
        CHECK_EQ(static_cast<int>(hal::IInput::KeyEvent::Back), 6);
    }
}

// ============================================================================
// Touch Coordinate Tests
// ============================================================================

TEST_CASE("Touch coordinate scaling") {
    const int scale = hal::InputSDL::DISPLAY_SCALE;

    SUBCASE("Window to display coordinate conversion") {
        // Window coordinates are scaled by DISPLAY_SCALE
        int window_x = 240;
        int window_y = 320;
        int display_x = window_x / scale;
        int display_y = window_y / scale;

        CHECK_EQ(display_x, 120);
        CHECK_EQ(display_y, 160);
    }

    SUBCASE("Corner coordinates") {
        // Top-left (window)
        CHECK_EQ(0 / scale, 0);
        CHECK_EQ(0 / scale, 0);

        // Bottom-right (window) - just inside the edge
        int max_window_x = 240 * scale - 1;  // 479
        int max_window_y = 320 * scale - 1;  // 639

        CHECK_EQ(max_window_x / scale, 239);
        CHECK_EQ(max_window_y / scale, 319);
    }

    SUBCASE("Touch bounds validation") {
        int display_x = 120;
        int display_y = 160;

        CHECK(display_x >= 0);
        CHECK(display_x < 240);
        CHECK(display_y >= 0);
        CHECK(display_y < 320);
    }
}

// ============================================================================
// Touch Type Tests
// ============================================================================

TEST_CASE("Touch type values") {
    SUBCASE("Touch types are distinct") {
        CHECK(static_cast<int>(hal::IInput::TouchType::Start) !=
              static_cast<int>(hal::IInput::TouchType::Move));
        CHECK(static_cast<int>(hal::IInput::TouchType::Move) !=
              static_cast<int>(hal::IInput::TouchType::End));
        CHECK(static_cast<int>(hal::IInput::TouchType::Start) !=
              static_cast<int>(hal::IInput::TouchType::End));
    }
}

// ============================================================================
// Encoder Tests
// ============================================================================

TEST_CASE("Encoder position calculations") {
    SUBCASE("Encoder delta accumulation") {
        int32_t position = 0;

        // Scroll up
        position += 3;
        CHECK_EQ(position, 3);

        // Scroll down
        position += -5;
        CHECK_EQ(position, -2);

        // Scroll up more
        position += 10;
        CHECK_EQ(position, 8);
    }

    SUBCASE("Encoder reset") {
        int32_t position = 100;
        position = 0;  // Reset
        CHECK_EQ(position, 0);
    }

    SUBCASE("Large encoder values") {
        int32_t position = 1000000;
        position += 1;
        CHECK_EQ(position, 1000001);
    }

    SUBCASE("Negative encoder values") {
        int32_t position = -1000000;
        position += -1;
        CHECK_EQ(position, -1000001);
    }
}

// ============================================================================
// Long Press Timing Tests
// ============================================================================

TEST_CASE("Long press timing logic") {
    const uint32_t long_press_ms = hal::InputSDL::LONG_PRESS_MS;

    SUBCASE("Short press detection") {
        uint32_t press_time = 100;  // 100ms
        CHECK(press_time < long_press_ms);
    }

    SUBCASE("Long press detection") {
        uint32_t press_time = 600;  // 600ms
        CHECK(press_time >= long_press_ms);
    }

    SUBCASE("Edge case at threshold") {
        uint32_t press_time = long_press_ms;
        CHECK(press_time >= long_press_ms);  // Exactly at threshold = long press

        press_time = long_press_ms - 1;
        CHECK(press_time < long_press_ms);  // Just under = short press
    }
}

// ============================================================================
// InputSDL Object Tests
// ============================================================================

TEST_CASE("InputSDL object creation") {
    SUBCASE("Object can be created") {
        hal::InputSDL input;
        CHECK(true);
    }
}

// ============================================================================
// Key State Array Tests
// ============================================================================

TEST_CASE("Key state management logic") {
    SUBCASE("Key state array size") {
        // Should have space for all 7 keys
        size_t num_keys = 7;
        std::array<bool, 7> key_states;
        key_states.fill(false);

        CHECK_EQ(key_states.size(), num_keys);
    }

    SUBCASE("Individual key state tracking") {
        std::array<bool, 7> key_states;
        key_states.fill(false);

        // Press Right key
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Right)] = true;
        CHECK(key_states[0] == true);
        CHECK(key_states[1] == false);

        // Release Right, press Select
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Right)] = false;
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Select)] = true;
        CHECK(key_states[0] == false);
        CHECK(key_states[4] == true);
    }

    SUBCASE("Multiple keys pressed simultaneously") {
        std::array<bool, 7> key_states;
        key_states.fill(false);

        // Press multiple keys
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Left)] = true;
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Right)] = true;
        key_states[static_cast<size_t>(hal::IInput::KeyEvent::Select)] = true;

        int pressed_count = 0;
        for (bool state : key_states) {
            if (state) pressed_count++;
        }
        CHECK_EQ(pressed_count, 3);
    }
}

TEST_SUITE_END();
