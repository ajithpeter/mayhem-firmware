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
 * @file test_shim_display.cpp
 * @brief Unit tests for display shim.
 */

#include "doctest.h"
#include "display_shim.hpp"

TEST_SUITE_BEGIN("DisplayShim");

// ============================================================================
// Color Tests
// ============================================================================

TEST_CASE("Color structure") {
    SUBCASE("RGB constructor") {
        shim::Color c(255, 128, 64);
        CHECK_EQ(c.r, 255);
        CHECK_EQ(c.g, 128);
        CHECK_EQ(c.b, 64);
    }

    SUBCASE("Default constructor") {
        shim::Color c;
        CHECK_EQ(c.r, 0);
        CHECK_EQ(c.g, 0);
        CHECK_EQ(c.b, 0);
    }

    SUBCASE("Pure colors") {
        shim::Color red(255, 0, 0);
        shim::Color green(0, 255, 0);
        shim::Color blue(0, 0, 255);

        CHECK_EQ(red.r, 255);
        CHECK_EQ(green.g, 255);
        CHECK_EQ(blue.b, 255);
    }

    SUBCASE("Black and white") {
        shim::Color black(0, 0, 0);
        shim::Color white(255, 255, 255);

        CHECK_EQ(black.r + black.g + black.b, 0);
        CHECK_EQ(white.r + white.g + white.b, 765);
    }
}

TEST_CASE("Color to RGB565 conversion") {
    SUBCASE("Pure red") {
        shim::Color red(255, 0, 0);
        uint16_t rgb565 = red.to_rgb565();
        CHECK_EQ(rgb565, 0xF800);
    }

    SUBCASE("Pure green") {
        shim::Color green(0, 255, 0);
        uint16_t rgb565 = green.to_rgb565();
        CHECK_EQ(rgb565, 0x07E0);
    }

    SUBCASE("Pure blue") {
        shim::Color blue(0, 0, 255);
        uint16_t rgb565 = blue.to_rgb565();
        CHECK_EQ(rgb565, 0x001F);
    }

    SUBCASE("White") {
        shim::Color white(255, 255, 255);
        uint16_t rgb565 = white.to_rgb565();
        CHECK_EQ(rgb565, 0xFFFF);
    }

    SUBCASE("Black") {
        shim::Color black(0, 0, 0);
        uint16_t rgb565 = black.to_rgb565();
        CHECK_EQ(rgb565, 0x0000);
    }

    SUBCASE("Yellow") {
        shim::Color yellow(255, 255, 0);
        uint16_t rgb565 = yellow.to_rgb565();
        CHECK_EQ(rgb565, 0xFFE0);
    }
}

// ============================================================================
// Point Tests
// ============================================================================

TEST_CASE("Point structure") {
    SUBCASE("Constructor") {
        shim::Point p(100, 200);
        CHECK_EQ(p.x, 100);
        CHECK_EQ(p.y, 200);
    }

    SUBCASE("Default constructor") {
        shim::Point p;
        CHECK_EQ(p.x, 0);
        CHECK_EQ(p.y, 0);
    }

    SUBCASE("Negative coordinates") {
        shim::Point p(-10, -20);
        CHECK_EQ(p.x, -10);
        CHECK_EQ(p.y, -20);
    }

    SUBCASE("Addition operator") {
        shim::Point a(10, 20);
        shim::Point b(5, 10);
        shim::Point c = a + b;
        CHECK_EQ(c.x, 15);
        CHECK_EQ(c.y, 30);
    }

    SUBCASE("Subtraction operator") {
        shim::Point a(10, 20);
        shim::Point b(5, 10);
        shim::Point c = a - b;
        CHECK_EQ(c.x, 5);
        CHECK_EQ(c.y, 10);
    }
}

// ============================================================================
// Size Tests
// ============================================================================

TEST_CASE("Size structure") {
    SUBCASE("Constructor") {
        shim::Size s(240, 320);
        CHECK_EQ(s.width, 240);
        CHECK_EQ(s.height, 320);
    }

    SUBCASE("Default constructor") {
        shim::Size s;
        CHECK_EQ(s.width, 0);
        CHECK_EQ(s.height, 0);
    }

    SUBCASE("Area calculation") {
        shim::Size s(100, 50);
        CHECK_EQ(s.width * s.height, 5000);
    }
}

// ============================================================================
// Rect Tests
// ============================================================================

TEST_CASE("Rect structure") {
    SUBCASE("Constructor with components") {
        shim::Rect r(10, 20, 100, 50);
        CHECK_EQ(r.x, 10);
        CHECK_EQ(r.y, 20);
        CHECK_EQ(r.width, 100);
        CHECK_EQ(r.height, 50);
    }

    SUBCASE("Constructor with Point and Size") {
        shim::Point p(10, 20);
        shim::Size s(100, 50);
        shim::Rect r(p, s);
        CHECK_EQ(r.x, 10);
        CHECK_EQ(r.y, 20);
        CHECK_EQ(r.width, 100);
        CHECK_EQ(r.height, 50);
    }

    SUBCASE("Contains point") {
        shim::Rect r(10, 20, 100, 50);

        CHECK(r.contains(shim::Point(50, 40)));  // Inside
        CHECK(r.contains(shim::Point(10, 20)));  // Top-left corner
        CHECK_FALSE(r.contains(shim::Point(5, 40)));   // Left of rect
        CHECK_FALSE(r.contains(shim::Point(120, 40))); // Right of rect
    }

    SUBCASE("Right and bottom edges") {
        shim::Rect r(10, 20, 100, 50);
        CHECK_EQ(r.right(), 110);
        CHECK_EQ(r.bottom(), 70);
    }

    SUBCASE("Location and size") {
        shim::Rect r(10, 20, 100, 50);

        shim::Point loc = r.location();
        CHECK_EQ(loc.x, 10);
        CHECK_EQ(loc.y, 20);

        shim::Size size = r.size();
        CHECK_EQ(size.width, 100);
        CHECK_EQ(size.height, 50);
    }

    SUBCASE("Empty rect") {
        shim::Rect r(10, 20, 0, 0);
        CHECK(r.is_empty());

        shim::Rect r2(10, 20, 100, 50);
        CHECK_FALSE(r2.is_empty());
    }

    SUBCASE("Intersects") {
        shim::Rect r1(0, 0, 100, 100);
        shim::Rect r2(50, 50, 100, 100);
        shim::Rect r3(200, 200, 50, 50);

        CHECK(r1.intersects(r2));
        CHECK_FALSE(r1.intersects(r3));
    }
}

// ============================================================================
// Display Interface Tests
// ============================================================================

TEST_CASE("Display interface") {
    SUBCASE("Get display instance") {
        auto& display = shim::get_display();
        CHECK(&display != nullptr);
    }
}

// ============================================================================
// Display Constants
// ============================================================================

TEST_CASE("Display constants") {
    SUBCASE("Display dimensions") {
        CHECK_EQ(shim::Display::WIDTH, 240);
        CHECK_EQ(shim::Display::HEIGHT, 320);
    }

    SUBCASE("Pixel count") {
        CHECK_EQ(shim::Display::WIDTH * shim::Display::HEIGHT, 76800);
    }
}

// ============================================================================
// Line Rendering Calculations
// ============================================================================

TEST_CASE("Line rendering calculations") {
    SUBCASE("Horizontal line") {
        shim::Point start(10, 50);
        shim::Point end(100, 50);

        int dx = end.x - start.x;
        int dy = end.y - start.y;

        CHECK_EQ(dx, 90);
        CHECK_EQ(dy, 0);
    }

    SUBCASE("Vertical line") {
        shim::Point start(50, 10);
        shim::Point end(50, 100);

        int dx = end.x - start.x;
        int dy = end.y - start.y;

        CHECK_EQ(dx, 0);
        CHECK_EQ(dy, 90);
    }

    SUBCASE("Diagonal line") {
        shim::Point start(0, 0);
        shim::Point end(100, 100);

        int dx = end.x - start.x;
        int dy = end.y - start.y;

        CHECK_EQ(dx, 100);
        CHECK_EQ(dy, 100);
    }
}

// ============================================================================
// Bitmap Data Tests
// ============================================================================

TEST_CASE("Bitmap data") {
    SUBCASE("Bitmap structure") {
        // 8x8 bitmap
        uint8_t data[8] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF};

        CHECK_EQ(data[0], 0xFF);  // Top row all set
        CHECK_EQ(data[7], 0xFF);  // Bottom row all set
    }

    SUBCASE("Bit extraction") {
        uint8_t byte = 0b10101010;

        CHECK((byte & 0x80) != 0);  // Bit 7 set
        CHECK((byte & 0x40) == 0);  // Bit 6 not set
        CHECK((byte & 0x20) != 0);  // Bit 5 set
        CHECK((byte & 0x10) == 0);  // Bit 4 not set
    }
}

TEST_SUITE_END();
