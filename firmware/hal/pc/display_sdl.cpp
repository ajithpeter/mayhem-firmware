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

#include "display_sdl.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace hal {

DisplaySDL::DisplaySDL() {
    pixels_.fill(0);  // Initialize to black
}

DisplaySDL::~DisplaySDL() {
    shutdown();
}

bool DisplaySDL::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    // Create window (avoid ALLOW_HIGHDPI to keep coordinate mapping simple)
    window_ = SDL_CreateWindow(
        "PortaPack Mayhem Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    renderer_ = SDL_CreateRenderer(
        window_,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Set render scale quality
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    // Create texture for framebuffer
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (!texture_) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        renderer_ = nullptr;
        window_ = nullptr;
        return false;
    }

    // Don't use SDL_RenderSetLogicalSize - it causes coordinate issues
    // We'll handle scaling manually in present()

    initialized_ = true;
    std::cout << "DisplaySDL initialized: " << WIDTH << "x" << HEIGHT
              << " (scaled to " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << ")" << std::endl;

    return true;
}

void DisplaySDL::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    initialized_ = false;
}

void DisplaySDL::fill_rectangle(int x, int y, int w, int h, uint16_t color) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    // Clip to display bounds
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);
    int x2 = std::min(WIDTH, x + w);
    int y2 = std::min(HEIGHT, y + h);

    for (int py = y1; py < y2; ++py) {
        for (int px = x1; px < x2; ++px) {
            pixels_[py * WIDTH + px] = color;
        }
    }
}

void DisplaySDL::draw_pixel(int x, int y, uint16_t color) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    set_pixel(x, y, color);
}

void DisplaySDL::draw_bitmap(int x, int y, const uint8_t* data, int w, int h,
                              uint16_t fg, uint16_t bg) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !data) return;

    // Calculate bytes per row (1 bit per pixel, rounded up to byte boundary)
    int bytes_per_row = (w + 7) / 8;

    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            int byte_idx = py * bytes_per_row + (px / 8);
            int bit_idx = 7 - (px % 8);  // MSB first
            bool pixel_set = (data[byte_idx] >> bit_idx) & 1;

            int draw_x = x + px;
            int draw_y = y + py;

            if (draw_x >= 0 && draw_x < WIDTH && draw_y >= 0 && draw_y < HEIGHT) {
                pixels_[draw_y * WIDTH + draw_x] = pixel_set ? fg : bg;
            }
        }
    }
}

void DisplaySDL::render_line(int x, int y, int count, const uint16_t* colors) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !colors) return;

    if (y < 0 || y >= HEIGHT) return;

    int x1 = std::max(0, x);
    int x2 = std::min(WIDTH, x + count);
    int offset = x1 - x;

    for (int px = x1; px < x2; ++px) {
        pixels_[y * WIDTH + px] = colors[offset + (px - x1)];
    }
}

void DisplaySDL::draw_hline(int x, int y, int w, uint16_t color) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    if (y < 0 || y >= HEIGHT) return;

    int x1 = std::max(0, x);
    int x2 = std::min(WIDTH, x + w);

    for (int px = x1; px < x2; ++px) {
        pixels_[y * WIDTH + px] = color;
    }
}

void DisplaySDL::draw_vline(int x, int y, int h, uint16_t color) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    if (x < 0 || x >= WIDTH) return;

    int y1 = std::max(0, y);
    int y2 = std::min(HEIGHT, y + h);

    for (int py = y1; py < y2; ++py) {
        pixels_[py * WIDTH + x] = color;
    }
}

void DisplaySDL::scroll_set_area(uint16_t top, uint16_t bottom) {
    std::lock_guard<std::mutex> lock(mutex_);

    scroll_top_ = std::min(top, static_cast<uint16_t>(HEIGHT));
    scroll_bottom_ = std::min(bottom, static_cast<uint16_t>(HEIGHT));
    scroll_offset_ = 0;
}

void DisplaySDL::scroll(int16_t delta) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    // For now, implement simple vertical scrolling by shifting the framebuffer
    // In a real implementation, this would work with the scroll area settings

    int scroll_height = scroll_bottom_ - scroll_top_;
    if (scroll_height <= 0) return;

    scroll_offset_ = (scroll_offset_ + delta) % scroll_height;
    if (scroll_offset_ < 0) {
        scroll_offset_ += scroll_height;
    }

    // Create a copy for rotation
    std::array<uint16_t, WIDTH * HEIGHT> temp_pixels;

    for (int y = scroll_top_; y < scroll_bottom_; ++y) {
        int src_y = scroll_top_ + ((y - scroll_top_ + scroll_offset_) % scroll_height);
        std::memcpy(&temp_pixels[y * WIDTH], &pixels_[src_y * WIDTH], WIDTH * sizeof(uint16_t));
    }

    // Copy back
    for (int y = scroll_top_; y < scroll_bottom_; ++y) {
        std::memcpy(&pixels_[y * WIDTH], &temp_pixels[y * WIDTH], WIDTH * sizeof(uint16_t));
    }
}

void DisplaySDL::present() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !texture_ || !renderer_) return;

    // Convert RGB565 framebuffer to ARGB8888 and update texture
    uint32_t* pixels32;
    int pitch;

    if (SDL_LockTexture(texture_, nullptr, reinterpret_cast<void**>(&pixels32), &pitch) == 0) {
        // Apply sleep dimming if needed
        float brightness = sleeping_ ? 0.1f : (backlight_level_ / 255.0f);

        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            uint32_t argb = rgb565_to_argb8888(pixels_[i]);
            if (brightness < 1.0f) {
                uint8_t r = static_cast<uint8_t>(((argb >> 16) & 0xFF) * brightness);
                uint8_t g = static_cast<uint8_t>(((argb >> 8) & 0xFF) * brightness);
                uint8_t b = static_cast<uint8_t>((argb & 0xFF) * brightness);
                argb = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
            pixels32[i] = argb;
        }
        SDL_UnlockTexture(texture_);
    }

    // Clear, copy texture, and present
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void DisplaySDL::sleep(bool sleep_enable) {
    sleeping_ = sleep_enable;
}

void DisplaySDL::set_backlight(uint8_t level) {
    backlight_level_ = level;
}

} // namespace hal
