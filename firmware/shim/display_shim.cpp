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

#include "display_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include <algorithm>
#include <cmath>

// Minimal UI type definitions for shim (matching ui.hpp)
namespace ui {
    struct Color {
        uint16_t v;
        constexpr Color() : v(0) {}
        constexpr Color(uint16_t v) : v(v) {}
    };

    struct Point {
        int16_t _x, _y;
        constexpr Point() : _x(0), _y(0) {}
        constexpr Point(int x, int y) : _x(x), _y(y) {}
        constexpr int x() const { return _x; }
        constexpr int y() const { return _y; }
    };

    struct Size {
        int16_t _w, _h;
        constexpr Size() : _w(0), _h(0) {}
        constexpr Size(int w, int h) : _w(w), _h(h) {}
        int width() const { return _w; }
        int height() const { return _h; }
    };

    struct Rect {
        Point _pos;
        Size _size;
        constexpr Rect() {}
        constexpr Rect(int x, int y, int w, int h) : _pos(x, y), _size(w, h) {}
        int left() const { return _pos.x(); }
        int top() const { return _pos.y(); }
        int right() const { return _pos.x() + _size.width(); }
        int bottom() const { return _pos.y() + _size.height(); }
        int width() const { return _size.width(); }
        int height() const { return _size.height(); }
    };

    struct Bitmap {
        Size size;
        const uint8_t* data;
    };
}

namespace shim {

static Display display_instance;

Display& get_display() {
    return display_instance;
}

void Display::init() {
    // Display is initialized in portapack_shim::init()
}

void Display::shutdown() {
    // Display is shutdown in portapack_shim::shutdown()
}

void Display::fill_rectangle(int x, int y, int w, int h, uint16_t color) {
    portapack_shim::display_hal.fill_rectangle(x, y, w, h, color);
}

void Display::fill_rectangle(const ui::Rect& rect, const ui::Color& color) {
    fill_rectangle(rect.left(), rect.top(), rect.width(), rect.height(), color.v);
}

void Display::draw_pixel(int x, int y, uint16_t color) {
    portapack_shim::display_hal.draw_pixel(x, y, color);
}

void Display::draw_pixel(const ui::Point& p, const ui::Color& color) {
    draw_pixel(p.x(), p.y(), color.v);
}

void Display::draw_hline(int x, int y, int w, uint16_t color) {
    portapack_shim::display_hal.draw_hline(x, y, w, color);
}

void Display::draw_vline(int x, int y, int h, uint16_t color) {
    portapack_shim::display_hal.draw_vline(x, y, h, color);
}

void Display::draw_line(const ui::Point& start, const ui::Point& end, const ui::Color& color) {
    // Bresenham's line algorithm
    int x0 = start.x();
    int y0 = start.y();
    int x1 = end.x();
    int y1 = end.y();

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        draw_pixel(x0, y0, color.v);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Display::draw_rectangle(const ui::Rect& rect, const ui::Color& color) {
    draw_hline(rect.left(), rect.top(), rect.width(), color.v);
    draw_hline(rect.left(), rect.bottom() - 1, rect.width(), color.v);
    draw_vline(rect.left(), rect.top(), rect.height(), color.v);
    draw_vline(rect.right() - 1, rect.top(), rect.height(), color.v);
}

void Display::draw_bitmap(int x, int y, const uint8_t* data, int w, int h,
                          uint16_t fg, uint16_t bg) {
    portapack_shim::display_hal.draw_bitmap(x, y, data, w, h, fg, bg);
}

void Display::draw_bitmap(const ui::Point& p, const ui::Bitmap& bitmap,
                          const ui::Color& fg, const ui::Color& bg) {
    draw_bitmap(p.x(), p.y(), bitmap.data,
                bitmap.size.width(), bitmap.size.height(),
                fg.v, bg.v);
}

void Display::drawBMP(const ui::Point& p, const uint8_t* bmp, bool transparency) {
    if (!bmp) return;

    // Parse BMP header (simplified - assumes valid BMP format)
    // BMP header: 2 bytes magic, 4 bytes file size, 4 bytes reserved,
    // 4 bytes data offset, then DIB header with width/height

    if (bmp[0] != 'B' || bmp[1] != 'M') return;

    uint32_t data_offset = bmp[10] | (bmp[11] << 8) | (bmp[12] << 16) | (bmp[13] << 24);
    int32_t width = bmp[18] | (bmp[19] << 8) | (bmp[20] << 16) | (bmp[21] << 24);
    int32_t height = bmp[22] | (bmp[23] << 8) | (bmp[24] << 16) | (bmp[25] << 24);
    uint16_t bits_per_pixel = bmp[28] | (bmp[29] << 8);

    bool bottom_up = height > 0;
    height = std::abs(height);

    const uint8_t* pixel_data = bmp + data_offset;

    // Only handle 16-bit and 24-bit BMPs for simplicity
    if (bits_per_pixel == 16) {
        // RGB565 format
        int row_stride = ((width * 2 + 3) / 4) * 4;

        for (int y = 0; y < height; ++y) {
            int src_y = bottom_up ? (height - 1 - y) : y;
            const uint16_t* row = reinterpret_cast<const uint16_t*>(pixel_data + src_y * row_stride);

            for (int x = 0; x < width; ++x) {
                uint16_t color = row[x];
                if (!transparency || color != 0xF81F) {  // Magenta = transparent
                    draw_pixel(p.x() + x, p.y() + y, color);
                }
            }
        }
    } else if (bits_per_pixel == 24) {
        // RGB888 format
        int row_stride = ((width * 3 + 3) / 4) * 4;

        for (int y = 0; y < height; ++y) {
            int src_y = bottom_up ? (height - 1 - y) : y;
            const uint8_t* row = pixel_data + src_y * row_stride;

            for (int x = 0; x < width; ++x) {
                uint8_t b = row[x * 3 + 0];
                uint8_t g = row[x * 3 + 1];
                uint8_t r = row[x * 3 + 2];

                // Convert to RGB565
                uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);

                if (!transparency || (r != 255 || g != 0 || b != 255)) {
                    draw_pixel(p.x() + x, p.y() + y, color);
                }
            }
        }
    }
}

void Display::render_line(int x, int y, int count, const uint16_t* colors) {
    portapack_shim::display_hal.render_line(x, y, count, colors);
}

void Display::render_line(const ui::Point& p, uint8_t count, const ui::Color* colors) {
    render_line(p.x(), p.y(), count, reinterpret_cast<const uint16_t*>(colors));
}

void Display::scroll_set_area(uint16_t top, uint16_t bottom) {
    portapack_shim::display_hal.scroll_set_area(top, bottom);
}

void Display::scroll(int16_t delta) {
    portapack_shim::display_hal.scroll(delta);
}

void Display::scroll_disable() {
    scroll_set_area(0, height());
}

void Display::sleep() {
    portapack_shim::display_hal.sleep(true);
}

void Display::wake() {
    portapack_shim::display_hal.sleep(false);
}

void Display::set_backlight(uint8_t level) {
    portapack_shim::display_hal.set_backlight(level);
}

void Display::present() {
    portapack_shim::display_hal.present();
}

} // namespace shim

#endif // PORTAPACK_PC_EMULATOR
