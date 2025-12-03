/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * PC-compatible ILI9341 display class that wraps the SDL display shim.
 * This allows the real PortaPack UI code to render to the PC display.
 */

#ifndef __LCD_ILI9341_PC_HPP__
#define __LCD_ILI9341_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include "ui.hpp"
#include "ui_text.hpp"
#include "display_shim.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <filesystem>

namespace lcd {

class ILI9341 {
public:
    ILI9341()
        : scroll_state{0, 0, height(), 0} {
    }

    ILI9341(const ILI9341&) = delete;
    ILI9341(ILI9341&&) = delete;
    void operator=(const ILI9341&) = delete;

    bool read_display_status() { return true; }
    uint32_t lcd_read_display_id() { return 0x9341; }

    void init() {}
    void shutdown() {}

    void sleep(bool = true) {}
    void wake(bool = true) {}

    void fill_rectangle(ui::Rect r, const ui::Color c) {
        auto& display = shim::get_display();
        display.fill_rectangle(r.left(), r.top(), r.width(), r.height(), c.v);
    }

    void fill_rectangle_unrolled8(ui::Rect r, const ui::Color c) {
        fill_rectangle(r, c);
    }

    void draw_line(const ui::Point start, const ui::Point end, const ui::Color color) {
        auto& display = shim::get_display();
        // Bresenham's line algorithm
        int x0 = start.x(), y0 = start.y();
        int x1 = end.x(), y1 = end.y();
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        while (true) {
            display.draw_pixel(x0, y0, color.v);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }

    void fill_circle(
        const ui::Point center,
        const ui::Dim radius,
        const ui::Color foreground,
        const ui::Color background) {
        // Simple filled circle using midpoint algorithm
        auto& display = shim::get_display();
        int cx = center.x(), cy = center.y();
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                if (x * x + y * y <= radius * radius) {
                    display.draw_pixel(cx + x, cy + y, foreground.v);
                }
            }
        }
    }

    void draw_pixel(const ui::Point p, const ui::Color color) {
        auto& display = shim::get_display();
        display.draw_pixel(p.x(), p.y(), color.v);
    }

    void draw_bmp_from_bmp_hex_arr(const ui::Point p, const uint8_t* bitmap, const uint8_t* transparency_color) {
        (void)p; (void)bitmap; (void)transparency_color;
        // Not implemented for PC
    }

    bool draw_bmp_from_sdcard_file(const ui::Point p, const std::filesystem::path& file) {
        (void)p; (void)file;
        return false; // Not implemented for PC
    }

    void render_line(const ui::Point p, const uint16_t count, const ui::Color* line_buffer) {
        auto& display = shim::get_display();
        for (uint16_t i = 0; i < count; ++i) {
            display.draw_pixel(p.x() + i, p.y(), line_buffer[i].v);
        }
    }

    void render_box(const ui::Point p, const ui::Size s, const ui::Color* line_buffer) {
        auto& display = shim::get_display();
        int idx = 0;
        for (int y = 0; y < s.height(); ++y) {
            for (int x = 0; x < s.width(); ++x) {
                display.draw_pixel(p.x() + x, p.y() + y, line_buffer[idx++].v);
            }
        }
    }

    template <size_t N>
    void draw_pixels(const ui::Rect r, const std::array<ui::Color, N>& colors) {
        draw_pixels(r, colors.data(), colors.size());
    }

    void draw_pixels(const ui::Rect r, const std::vector<ui::Color>& colors) {
        draw_pixels(r, colors.data(), colors.size());
    }

    void draw_pixels(const ui::Rect r, const ui::Color* const colors, const size_t count) {
        auto& display = shim::get_display();
        size_t idx = 0;
        for (int y = r.top(); y < r.bottom() && idx < count; ++y) {
            for (int x = r.left(); x < r.right() && idx < count; ++x) {
                display.draw_pixel(x, y, colors[idx++].v);
            }
        }
    }

    template <size_t N>
    void read_pixels(const ui::Rect r, std::array<ui::ColorRGB888, N>& colors) {
        read_pixels(r, colors.data(), colors.size());
    }

    void read_pixels(const ui::Rect r, std::vector<ui::ColorRGB888>& colors) {
        read_pixels(r, colors.data(), colors.size());
    }

    void read_pixels(const ui::Rect r, ui::ColorRGB888* const colors, const size_t count) {
        (void)r; (void)colors; (void)count;
        // Not implemented - would need to read back from SDL texture
    }

    void draw_bitmap(
        const ui::Point p,
        const ui::Size size,
        const uint8_t* const data,
        const ui::Color foreground,
        const ui::Color background,
        uint8_t zoom_level = 1) {

        auto& display = shim::get_display();
        const int bytes_per_row = (size.width() + 7) / 8;

        for (int zy = 0; zy < size.height(); ++zy) {
            for (int zx = 0; zx < size.width(); ++zx) {
                const int byte_idx = zy * bytes_per_row + (zx / 8);
                const int bit_idx = 7 - (zx % 8);
                const bool pixel_set = (data[byte_idx] >> bit_idx) & 1;
                const ui::Color color = pixel_set ? foreground : background;

                // Handle zoom
                for (int dy = 0; dy < zoom_level; ++dy) {
                    for (int dx = 0; dx < zoom_level; ++dx) {
                        display.draw_pixel(
                            p.x() + zx * zoom_level + dx,
                            p.y() + zy * zoom_level + dy,
                            color.v
                        );
                    }
                }
            }
        }
    }

    void draw_glyph(
        const ui::Point p,
        const ui::Glyph& glyph,
        const ui::Color foreground,
        const ui::Color background,
        uint8_t zoom_level = 1) {

        draw_bitmap(p, glyph.size(), glyph.pixels(), foreground, background, zoom_level);
    }

    void scroll_set_area(const ui::Coord top_y, const ui::Coord bottom_y) {
        scroll_state.top_area = top_y;
        scroll_state.bottom_area = bottom_y;
        scroll_state.height = bottom_y - top_y;
    }

    void scroll_disable() {
        scroll_state = {0, 0, height(), 0};
    }

    ui::Coord scroll_set_position(const ui::Coord position) {
        scroll_state.current_position = position % scroll_state.height;
        return scroll_state.current_position;
    }

    ui::Coord scroll(const int32_t delta) {
        return scroll_set_position(scroll_state.current_position + delta);
    }

    ui::Coord scroll_area_y(const ui::Coord y) const {
        const auto wrapped = (scroll_state.current_position + y) % scroll_state.height;
        return scroll_state.top_area + wrapped;
    }

    ui::Dim width() { return ui::screen_width; }
    ui::Dim height() { return ui::screen_height; }
    ui::Rect screen_rect() { return {0, 0, width(), height()}; }

private:
    struct scroll_t {
        ui::Coord top_area;
        ui::Coord bottom_area;
        ui::Dim height;
        ui::Coord current_position;
    };
    scroll_t scroll_state;
};

} // namespace lcd

#endif // PORTAPACK_PC_EMULATOR

#endif // __LCD_ILI9341_PC_HPP__
