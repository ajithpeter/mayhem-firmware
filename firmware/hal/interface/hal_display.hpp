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

#ifndef __HAL_DISPLAY_HPP__
#define __HAL_DISPLAY_HPP__

#include <cstdint>
#include <cstddef>

namespace hal {

/**
 * @brief Abstract interface for display hardware abstraction layer.
 *
 * This interface defines the contract that all display implementations
 * must fulfill, allowing the application to work with different display
 * backends (ILI9341 LCD, SDL2 window, etc.)
 */
class IDisplay {
public:
    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 320;

    virtual ~IDisplay() = default;

    /**
     * @brief Initialize the display hardware
     * @return true on success, false on failure
     */
    virtual bool init() = 0;

    /**
     * @brief Shutdown the display
     */
    virtual void shutdown() = 0;

    /**
     * @brief Fill a rectangle with a solid color
     * @param x Left position
     * @param y Top position
     * @param w Width
     * @param h Height
     * @param color RGB565 color value
     */
    virtual void fill_rectangle(int x, int y, int w, int h, uint16_t color) = 0;

    /**
     * @brief Draw a single pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @param color RGB565 color value
     */
    virtual void draw_pixel(int x, int y, uint16_t color) = 0;

    /**
     * @brief Draw a 1-bit per pixel bitmap
     * @param x Left position
     * @param y Top position
     * @param data Bitmap data (1bpp, packed)
     * @param w Width in pixels
     * @param h Height in pixels
     * @param fg Foreground color (RGB565)
     * @param bg Background color (RGB565)
     */
    virtual void draw_bitmap(int x, int y, const uint8_t* data, int w, int h,
                             uint16_t fg, uint16_t bg) = 0;

    /**
     * @brief Render a horizontal line of pixels
     * @param x Start X coordinate
     * @param y Y coordinate
     * @param count Number of pixels
     * @param colors Array of RGB565 color values
     */
    virtual void render_line(int x, int y, int count, const uint16_t* colors) = 0;

    /**
     * @brief Draw a horizontal line
     * @param x Start X coordinate
     * @param y Y coordinate
     * @param w Width (length)
     * @param color RGB565 color value
     */
    virtual void draw_hline(int x, int y, int w, uint16_t color) = 0;

    /**
     * @brief Draw a vertical line
     * @param x X coordinate
     * @param y Start Y coordinate
     * @param h Height (length)
     * @param color RGB565 color value
     */
    virtual void draw_vline(int x, int y, int h, uint16_t color) = 0;

    /**
     * @brief Set the scrolling area
     * @param top Top fixed area height
     * @param bottom Bottom fixed area height
     */
    virtual void scroll_set_area(uint16_t top, uint16_t bottom) = 0;

    /**
     * @brief Scroll the display vertically
     * @param delta Number of lines to scroll (positive = down, negative = up)
     */
    virtual void scroll(int16_t delta) = 0;

    /**
     * @brief Present the framebuffer to the display
     * This is called after all drawing operations are complete for a frame.
     * On immediate-mode displays (like ILI9341), this may be a no-op.
     * On buffered displays (like SDL), this flips the backbuffer.
     */
    virtual void present() = 0;

    /**
     * @brief Set display sleep mode
     * @param sleep true to enter sleep, false to wake
     */
    virtual void sleep(bool sleep_enable) = 0;

    /**
     * @brief Set display backlight level
     * @param level 0-255 brightness level
     */
    virtual void set_backlight(uint8_t level) = 0;
};

} // namespace hal

#endif // __HAL_DISPLAY_HPP__
