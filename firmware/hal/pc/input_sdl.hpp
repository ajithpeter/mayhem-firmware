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

#ifndef __INPUT_SDL_HPP__
#define __INPUT_SDL_HPP__

#include "../interface/hal_input.hpp"
#include <SDL2/SDL.h>
#include <atomic>
#include <mutex>
#include <array>
#include <chrono>

namespace hal {

/**
 * @brief SDL2-based input implementation for PC emulation.
 *
 * This class implements the IInput interface using SDL2,
 * mapping keyboard keys and mouse events to PortaPack inputs:
 * - Arrow keys → Direction buttons
 * - Enter/Return → Select
 * - Escape/Backspace → Back
 * - Mouse wheel → Encoder rotation
 * - Mouse click → Touch events
 */
class InputSDL : public IInput {
public:
    static constexpr int DISPLAY_SCALE = 2;  // Match display scale
    static constexpr uint32_t LONG_PRESS_MS = 500;

    InputSDL();
    ~InputSDL() override;

    // IInput interface implementation
    bool init() override;
    void shutdown() override;
    void poll() override;
    bool is_key_pressed(KeyEvent key) const override;
    bool is_key_long_pressed(KeyEvent key) const override;
    int32_t encoder_position() const override;
    void reset_encoder() override;
    bool get_touch(int16_t& x, int16_t& y) const override;
    bool is_touching() const override;
    void set_key_callback(KeyCallback callback) override;
    void set_encoder_callback(EncoderCallback callback) override;
    void set_touch_callback(TouchCallback callback) override;
    bool quit_requested() const override;

private:
    void handle_key_event(const SDL_KeyboardEvent& event);
    void handle_mouse_button_event(const SDL_MouseButtonEvent& event);
    void handle_mouse_motion_event(const SDL_MouseMotionEvent& event);
    void handle_mouse_wheel_event(const SDL_MouseWheelEvent& event);

    KeyEvent sdl_key_to_event(SDL_Keycode key) const;

    // Key state
    std::array<bool, 7> key_pressed_;
    std::array<std::chrono::steady_clock::time_point, 7> key_press_time_;

    // Encoder state
    std::atomic<int32_t> encoder_position_{0};

    // Touch state
    std::atomic<int16_t> touch_x_{0};
    std::atomic<int16_t> touch_y_{0};
    std::atomic<bool> touching_{false};

    // Callbacks
    KeyCallback key_callback_;
    EncoderCallback encoder_callback_;
    TouchCallback touch_callback_;

    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> quit_requested_{false};

    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace hal

#endif // __INPUT_SDL_HPP__
