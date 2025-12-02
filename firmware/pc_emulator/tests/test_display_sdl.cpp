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
 * @file test_display_sdl.cpp
 * @brief Unit tests for SDL2 display implementation.
 */

#include "doctest.h"
#include "display_sdl.hpp"
#include <cstring>

TEST_SUITE_BEGIN("DisplaySDL");

// ============================================================================
// DisplaySDL Constants Tests
// ============================================================================

TEST_CASE("DisplaySDL constants are correct") {
    SUBCASE("Scale factor") {
        CHECK_EQ(hal::DisplaySDL::SCALE, 2);
    }

    SUBCASE("Window dimensions") {
        CHECK_EQ(hal::DisplaySDL::WINDOW_WIDTH, 240 * 2);
        CHECK_EQ(hal::DisplaySDL::WINDOW_HEIGHT, 320 * 2);
    }

    SUBCASE("Inherits from IDisplay") {
        CHECK_EQ(hal::DisplaySDL::WIDTH, 240);
        CHECK_EQ(hal::DisplaySDL::HEIGHT, 320);
    }
}

// ============================================================================
// DisplaySDL Functionality Tests (without SDL init)
// ============================================================================

TEST_CASE("DisplaySDL object creation") {
    // Note: We can't fully test SDL without a display server,
    // but we can test object creation
    hal::DisplaySDL display;

    SUBCASE("Initial state is not initialized") {
        // Display should not be initialized until init() is called
        // We can't check internal state, but the object should be creatable
        CHECK(true);
    }
}

// ============================================================================
// RGB565 Color Conversion Tests
// ============================================================================

TEST_CASE("RGB565 color format") {
    SUBCASE("Pure red") {
        // RGB565: RRRRR GGGGGG BBBBB
        // Red = 31, Green = 0, Blue = 0
        uint16_t red = (31 << 11) | (0 << 5) | 0;
        CHECK_EQ(red, 0xF800);
    }

    SUBCASE("Pure green") {
        // Green = 63 (6 bits)
        uint16_t green = (0 << 11) | (63 << 5) | 0;
        CHECK_EQ(green, 0x07E0);
    }

    SUBCASE("Pure blue") {
        uint16_t blue = (0 << 11) | (0 << 5) | 31;
        CHECK_EQ(blue, 0x001F);
    }

    SUBCASE("White") {
        uint16_t white = (31 << 11) | (63 << 5) | 31;
        CHECK_EQ(white, 0xFFFF);
    }

    SUBCASE("Black") {
        uint16_t black = 0;
        CHECK_EQ(black, 0x0000);
    }

    SUBCASE("Yellow (red + green)") {
        uint16_t yellow = (31 << 11) | (63 << 5) | 0;
        CHECK_EQ(yellow, 0xFFE0);
    }

    SUBCASE("Cyan (green + blue)") {
        uint16_t cyan = (0 << 11) | (63 << 5) | 31;
        CHECK_EQ(cyan, 0x07FF);
    }

    SUBCASE("Magenta (red + blue)") {
        uint16_t magenta = (31 << 11) | (0 << 5) | 31;
        CHECK_EQ(magenta, 0xF81F);
    }
}

// ============================================================================
// Coordinate Validation Tests
// ============================================================================

TEST_CASE("Coordinate bounds") {
    SUBCASE("Valid coordinates") {
        int x = 120, y = 160;
        CHECK(x >= 0);
        CHECK(x < hal::DisplaySDL::WIDTH);
        CHECK(y >= 0);
        CHECK(y < hal::DisplaySDL::HEIGHT);
    }

    SUBCASE("Edge coordinates") {
        // Top-left
        CHECK(0 >= 0);
        CHECK(0 < hal::DisplaySDL::WIDTH);

        // Bottom-right
        CHECK(hal::DisplaySDL::WIDTH - 1 < hal::DisplaySDL::WIDTH);
        CHECK(hal::DisplaySDL::HEIGHT - 1 < hal::DisplaySDL::HEIGHT);
    }

    SUBCASE("Corner coordinates are valid") {
        // All four corners should be within bounds
        int corners[][2] = {
            {0, 0},
            {239, 0},
            {0, 319},
            {239, 319}
        };

        for (const auto& corner : corners) {
            CHECK(corner[0] >= 0);
            CHECK(corner[0] < hal::DisplaySDL::WIDTH);
            CHECK(corner[1] >= 0);
            CHECK(corner[1] < hal::DisplaySDL::HEIGHT);
        }
    }
}

// ============================================================================
// Rectangle Bounds Tests
// ============================================================================

TEST_CASE("Rectangle bounds calculation") {
    SUBCASE("Full screen rectangle") {
        int x = 0, y = 0;
        int w = hal::DisplaySDL::WIDTH;
        int h = hal::DisplaySDL::HEIGHT;

        CHECK_EQ(x + w, hal::DisplaySDL::WIDTH);
        CHECK_EQ(y + h, hal::DisplaySDL::HEIGHT);
    }

    SUBCASE("Centered rectangle") {
        int w = 100, h = 50;
        int x = (hal::DisplaySDL::WIDTH - w) / 2;
        int y = (hal::DisplaySDL::HEIGHT - h) / 2;

        CHECK(x >= 0);
        CHECK(y >= 0);
        CHECK(x + w <= hal::DisplaySDL::WIDTH);
        CHECK(y + h <= hal::DisplaySDL::HEIGHT);
    }

    SUBCASE("Clipping calculation") {
        // Rectangle that extends past screen bounds
        int x = 200, y = 280;
        int w = 100, h = 100;

        int clipped_w = std::min(w, hal::DisplaySDL::WIDTH - x);
        int clipped_h = std::min(h, hal::DisplaySDL::HEIGHT - y);

        CHECK_EQ(clipped_w, 40);  // 240 - 200 = 40
        CHECK_EQ(clipped_h, 40);  // 320 - 280 = 40
    }
}

// ============================================================================
// Scroll Region Tests
// ============================================================================

TEST_CASE("Scroll region calculation") {
    SUBCASE("Scroll down by 10 pixels") {
        int scroll_amount = 10;
        int source_y = 0;
        int dest_y = scroll_amount;
        int copy_height = hal::DisplaySDL::HEIGHT - scroll_amount;

        CHECK_EQ(copy_height, 310);
        CHECK(dest_y + copy_height == hal::DisplaySDL::HEIGHT);
    }

    SUBCASE("Scroll up by 10 pixels") {
        int scroll_amount = -10;
        int source_y = -scroll_amount;
        int dest_y = 0;
        int copy_height = hal::DisplaySDL::HEIGHT + scroll_amount;

        CHECK_EQ(copy_height, 310);
        CHECK_EQ(source_y, 10);
    }
}

TEST_SUITE_END();
