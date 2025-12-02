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

#ifndef __DISPLAY_SHIM_HPP__
#define __DISPLAY_SHIM_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <algorithm>

// Forward declarations for UI types
namespace ui {
    struct Color;
    struct Point;
    struct Rect;
    struct Bitmap;
}

namespace shim {

/**
 * @brief RGB Color structure for PC emulation.
 */
struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    constexpr Color() = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

    // Convert to RGB565 format
    constexpr uint16_t to_rgb565() const {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }

    // Create from RGB565
    static constexpr Color from_rgb565(uint16_t rgb565) {
        return Color(
            ((rgb565 >> 11) & 0x1F) << 3,
            ((rgb565 >> 5) & 0x3F) << 2,
            (rgb565 & 0x1F) << 3
        );
    }
};

/**
 * @brief 2D Point structure.
 */
struct Point {
    int16_t x = 0;
    int16_t y = 0;

    constexpr Point() = default;
    constexpr Point(int16_t x_, int16_t y_) : x(x_), y(y_) {}

    constexpr Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }

    constexpr Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }

    constexpr bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Size structure (width/height).
 */
struct Size {
    int16_t width = 0;
    int16_t height = 0;

    constexpr Size() = default;
    constexpr Size(int16_t w, int16_t h) : width(w), height(h) {}
};

/**
 * @brief Rectangle structure.
 */
struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;

    constexpr Rect() = default;
    constexpr Rect(int16_t x_, int16_t y_, int16_t w_, int16_t h_)
        : x(x_), y(y_), width(w_), height(h_) {}
    constexpr Rect(const Point& p, const Size& s)
        : x(p.x), y(p.y), width(s.width), height(s.height) {}

    constexpr int16_t right() const { return x + width; }
    constexpr int16_t bottom() const { return y + height; }
    constexpr Point location() const { return Point(x, y); }
    constexpr Size size() const { return Size(width, height); }
    constexpr bool is_empty() const { return width <= 0 || height <= 0; }

    constexpr bool contains(const Point& p) const {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }

    constexpr bool intersects(const Rect& other) const {
        return !(other.x >= right() || other.right() <= x ||
                 other.y >= bottom() || other.bottom() <= y);
    }
};

/**
 * @brief Display shim class that wraps the PC display HAL.
 *
 * This class provides the same interface as lcd::ILI9341
 * but routes all calls to the SDL-based display implementation.
 */
class Display {
public:
    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 320;
    static constexpr int width() { return WIDTH; }
    static constexpr int height() { return HEIGHT; }

    Display() = default;
    ~Display() = default;

    // Initialization
    void init();
    void shutdown();

    // Basic drawing
    void fill_rectangle(int x, int y, int w, int h, uint16_t color);
    void fill_rectangle(int x, int y, int w, int h, const Color& color);
    void fill_rectangle(const ui::Rect& rect, const ui::Color& color);
    void draw_pixel(int x, int y, uint16_t color);
    void draw_pixel(const ui::Point& p, const ui::Color& color);

    // Line drawing
    void draw_hline(int x, int y, int w, uint16_t color);
    void draw_vline(int x, int y, int h, uint16_t color);
    void draw_line(const ui::Point& start, const ui::Point& end, const ui::Color& color);
    void draw_rectangle(const ui::Rect& rect, const ui::Color& color);

    // Bitmap drawing
    void draw_bitmap(int x, int y, const uint8_t* data, int w, int h,
                     uint16_t fg, uint16_t bg);
    void draw_bitmap(const ui::Point& p, const ui::Bitmap& bitmap,
                     const ui::Color& fg, const ui::Color& bg);

    // BMP file drawing
    void drawBMP(const ui::Point& p, const uint8_t* bmp, bool transparency);

    // Direct pixel rendering
    void render_line(int x, int y, int count, const uint16_t* colors);
    void render_line(const ui::Point& p, uint8_t count, const ui::Color* colors);

    // Scrolling
    void scroll_set_area(uint16_t top, uint16_t bottom);
    void scroll(int16_t delta);
    void scroll_disable();

    // Display control
    void sleep();
    void wake();
    void set_backlight(uint8_t level);

    // Frame presentation (call at end of frame)
    void present();
};

/**
 * @brief Get the global display shim instance.
 * @return Reference to the display shim
 */
Display& get_display();

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __DISPLAY_SHIM_HPP__
