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

#include "input_sdl.hpp"
#include <iostream>

namespace hal {

InputSDL::InputSDL() {
    key_pressed_.fill(false);
}

InputSDL::~InputSDL() {
    shutdown();
}

bool InputSDL::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    // SDL should already be initialized by display, but ensure events are enabled
    if (SDL_WasInit(SDL_INIT_EVENTS) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0) {
            std::cerr << "SDL Events init failed: " << SDL_GetError() << std::endl;
            return false;
        }
    }

    initialized_ = true;
    std::cout << "InputSDL initialized" << std::endl;
    return true;
}

void InputSDL::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
}

void InputSDL::poll() {
    if (!initialized_) return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit_requested_ = true;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handle_key_event(event.key);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                handle_mouse_button_event(event.button);
                break;

            case SDL_MOUSEMOTION:
                handle_mouse_motion_event(event.motion);
                break;

            case SDL_MOUSEWHEEL:
                handle_mouse_wheel_event(event.wheel);
                break;

            default:
                break;
        }
    }
}

void InputSDL::handle_key_event(const SDL_KeyboardEvent& event) {
    // Ignore key repeats for button state
    if (event.repeat) {
        return;
    }

    bool pressed = (event.type == SDL_KEYDOWN);
    KeyEvent key_event;

    // Map SDL keys to PortaPack buttons
    switch (event.keysym.sym) {
        case SDLK_RIGHT:
            key_event = KeyEvent::Right;
            break;
        case SDLK_LEFT:
            key_event = KeyEvent::Left;
            break;
        case SDLK_DOWN:
            key_event = KeyEvent::Down;
            break;
        case SDLK_UP:
            key_event = KeyEvent::Up;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            key_event = KeyEvent::Select;
            break;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
            key_event = KeyEvent::Back;
            break;
        case SDLK_d:  // DFU mode key
            key_event = KeyEvent::Dfu;
            break;
        default:
            return;  // Unknown key, ignore
    }

    size_t key_idx = static_cast<size_t>(key_event);
    if (key_idx >= key_pressed_.size()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    key_pressed_[key_idx] = pressed;

    if (pressed) {
        key_press_time_[key_idx] = std::chrono::steady_clock::now();
    }

    // Invoke callback
    if (key_callback_) {
        key_callback_(key_event, pressed);
    }
}

void InputSDL::handle_mouse_button_event(const SDL_MouseButtonEvent& event) {
    if (event.button != SDL_BUTTON_LEFT) return;

    // Get window size to compute proper scaling
    SDL_Window* win = SDL_GetWindowFromID(event.windowID);
    int win_w = 240, win_h = 320;  // Default to display size
    if (win) {
        SDL_GetWindowSize(win, &win_w, &win_h);
        // Ensure we don't divide by zero
        if (win_w <= 0) win_w = 240;
        if (win_h <= 0) win_h = 320;
    }

    // Debug: print raw values
    std::cout << "DEBUG: raw=(" << event.x << "," << event.y << ") win=(" << win_w << "," << win_h << ")" << std::endl;

    // Scale from window coordinates to display coordinates (240x320)
    int16_t x = static_cast<int16_t>((event.x * 240) / win_w);
    int16_t y = static_cast<int16_t>((event.y * 320) / win_h);
    x = std::max<int16_t>(0, std::min<int16_t>(239, x));
    y = std::max<int16_t>(0, std::min<int16_t>(319, y));

    bool pressed = (event.type == SDL_MOUSEBUTTONDOWN);

    std::lock_guard<std::mutex> lock(mutex_);

    touch_x_ = x;
    touch_y_ = y;
    touching_ = pressed;

    if (touch_callback_) {
        TouchEvent touch_event;
        touch_event.x = x;
        touch_event.y = y;
        touch_event.type = pressed ? TouchType::Start : TouchType::End;
        touch_callback_(touch_event);
    }
}

void InputSDL::handle_mouse_motion_event(const SDL_MouseMotionEvent& event) {
    if (!(event.state & SDL_BUTTON_LMASK)) return;  // Only track when button pressed

    // Get window size to compute proper scaling
    SDL_Window* win = SDL_GetWindowFromID(event.windowID);
    int win_w = 240, win_h = 320;
    if (win) {
        SDL_GetWindowSize(win, &win_w, &win_h);
        if (win_w <= 0) win_w = 240;
        if (win_h <= 0) win_h = 320;
    }

    // Scale from window coordinates to display coordinates (240x320)
    int16_t x = static_cast<int16_t>((event.x * 240) / win_w);
    int16_t y = static_cast<int16_t>((event.y * 320) / win_h);
    x = std::max<int16_t>(0, std::min<int16_t>(239, x));
    y = std::max<int16_t>(0, std::min<int16_t>(319, y));

    std::lock_guard<std::mutex> lock(mutex_);

    touch_x_ = x;
    touch_y_ = y;

    if (touch_callback_) {
        TouchEvent touch_event;
        touch_event.x = x;
        touch_event.y = y;
        touch_event.type = TouchType::Move;
        touch_callback_(touch_event);
    }
}

void InputSDL::handle_mouse_wheel_event(const SDL_MouseWheelEvent& event) {
    int32_t delta = event.y;

    // Handle inverted scrolling
    if (event.direction == SDL_MOUSEWHEEL_FLIPPED) {
        delta = -delta;
    }

    encoder_position_ += delta;

    if (encoder_callback_) {
        encoder_callback_(delta);
    }
}

bool InputSDL::is_key_pressed(KeyEvent key) const {
    size_t key_idx = static_cast<size_t>(key);
    if (key_idx >= key_pressed_.size()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    return key_pressed_[key_idx];
}

bool InputSDL::is_key_long_pressed(KeyEvent key) const {
    size_t key_idx = static_cast<size_t>(key);
    if (key_idx >= key_pressed_.size()) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    if (!key_pressed_[key_idx]) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - key_press_time_[key_idx]
    ).count();

    return elapsed >= LONG_PRESS_MS;
}

int32_t InputSDL::encoder_position() const {
    return encoder_position_;
}

void InputSDL::reset_encoder() {
    encoder_position_ = 0;
}

bool InputSDL::get_touch(int16_t& x, int16_t& y) const {
    if (!touching_) return false;

    x = touch_x_;
    y = touch_y_;
    return true;
}

bool InputSDL::is_touching() const {
    return touching_;
}

void InputSDL::set_key_callback(KeyCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    key_callback_ = callback;
}

void InputSDL::set_encoder_callback(EncoderCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    encoder_callback_ = callback;
}

void InputSDL::set_touch_callback(TouchCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    touch_callback_ = callback;
}

bool InputSDL::quit_requested() const {
    return quit_requested_;
}

InputSDL::KeyEvent InputSDL::sdl_key_to_event(SDL_Keycode key) const {
    switch (key) {
        case SDLK_RIGHT:      return KeyEvent::Right;
        case SDLK_LEFT:       return KeyEvent::Left;
        case SDLK_DOWN:       return KeyEvent::Down;
        case SDLK_UP:         return KeyEvent::Up;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:   return KeyEvent::Select;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:  return KeyEvent::Back;
        default:              return KeyEvent::Select;  // Default fallback
    }
}

} // namespace hal
