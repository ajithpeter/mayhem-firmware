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

#ifndef __DISPLAY_SDL_HPP__
#define __DISPLAY_SDL_HPP__

#include "../interface/hal_display.hpp"
#include <SDL2/SDL.h>
#include <mutex>
#include <array>
#include <atomic>

namespace hal {

/**
 * @brief SDL2-based display implementation for PC emulation.
 *
 * This class implements the IDisplay interface using SDL2,
 * providing a windowed display that emulates the 240x320 ILI9341 LCD
 * of the PortaPack.
 */
class DisplaySDL : public IDisplay {
public:
    static constexpr int SCALE = 2;  // 2x scaling for better visibility
    static constexpr int WINDOW_WIDTH = WIDTH * SCALE;
    static constexpr int WINDOW_HEIGHT = HEIGHT * SCALE;

    DisplaySDL();
    ~DisplaySDL() override;

    // IDisplay interface implementation
    bool init() override;
    void shutdown() override;
    void fill_rectangle(int x, int y, int w, int h, uint16_t color) override;
    void draw_pixel(int x, int y, uint16_t color) override;
    void draw_bitmap(int x, int y, const uint8_t* data, int w, int h,
                     uint16_t fg, uint16_t bg) override;
    void render_line(int x, int y, int count, const uint16_t* colors) override;
    void draw_hline(int x, int y, int w, uint16_t color) override;
    void draw_vline(int x, int y, int h, uint16_t color) override;
    void scroll_set_area(uint16_t top, uint16_t bottom) override;
    void scroll(int16_t delta) override;
    void present() override;
    void sleep(bool sleep_enable) override;
    void set_backlight(uint8_t level) override;

    // Additional methods for emulator
    SDL_Window* window() const { return window_; }
    SDL_Renderer* renderer() const { return renderer_; }

    // Get the framebuffer for direct access if needed
    const uint16_t* framebuffer() const { return pixels_.data(); }

private:
    // Convert RGB565 to ARGB8888
    static uint32_t rgb565_to_argb8888(uint16_t c) {
        uint8_t r = ((c >> 11) & 0x1F) << 3;
        uint8_t g = ((c >> 5) & 0x3F) << 2;
        uint8_t b = (c & 0x1F) << 3;
        // Expand lower bits for better color representation
        r |= r >> 5;
        g |= g >> 6;
        b |= b >> 5;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    // Set a pixel in the framebuffer (with bounds checking)
    void set_pixel(int x, int y, uint16_t color) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            pixels_[y * WIDTH + x] = color;
        }
    }

    // SDL objects
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;

    // Framebuffer (RGB565 format matching ILI9341)
    std::array<uint16_t, WIDTH * HEIGHT> pixels_;

    // Thread safety
    mutable std::mutex mutex_;

    // Scroll parameters
    uint16_t scroll_top_ = 0;
    uint16_t scroll_bottom_ = HEIGHT;
    int16_t scroll_offset_ = 0;

    // Display state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> sleeping_{false};
    uint8_t backlight_level_ = 255;
};

} // namespace hal

#endif // __DISPLAY_SDL_HPP__
