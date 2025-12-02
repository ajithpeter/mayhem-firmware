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

/**
 * @file portapack_pc.cpp
 * @brief PC emulator implementation of portapack namespace globals.
 *
 * This file provides PC-compatible implementations of the hardware
 * interfaces defined in portapack.hpp, delegating to the HAL shim layer.
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "portapack_shim.hpp"
#include "display_shim.hpp"
#include "audio_shim.hpp"
#include "receiver_model_shim.hpp"
#include "transmitter_model_shim.hpp"
#include "shared_memory_shim.hpp"
#include "event_shim.hpp"

#include <iostream>
#include <memory>

// Forward declarations
class EventDispatcher_HW;  // The real hardware event dispatcher (we don't use it)

// Abstract Backlight interface for compatibility
class Backlight {
public:
    virtual ~Backlight() = default;
    virtual void on() = 0;
    virtual void off() = 0;
    virtual void set_level(uint8_t) = 0;
    virtual uint8_t level() const = 0;
};

namespace portapack {

// Initialization status enum
enum class init_status_t {
    INIT_SUCCESS,
    INIT_NO_PORTAPACK,
    INIT_PORTAPACK_CPLD_FAILED,
    INIT_HACKRF_CPLD_FAILED,
};

// PC implementation of IO class (stub)
class IO_PC {
public:
    void init() {}
    void lcd_data(uint8_t) {}
    void lcd_command(uint8_t) {}
    void lcd_backlight(bool) {}
    void audio_shutdown(bool) {}
    void reference_oscillator(bool) {}
    void lcd_reset() {}
    void touch_rst(bool) {}
    uint8_t touch_int() { return 0; }
    void lcd_cs(bool) {}
    void lcd_dc(bool) {}
    void lcd_wr(bool) {}
    void lcd_rd(bool) {}
    void lcd_data_write(uint16_t) {}
    uint16_t lcd_data_read() { return 0; }
    void lcd_data_write_begin() {}
    void lcd_data_write_end() {}
};

// PC implementation of I2C (stub)
class I2C_PC {
public:
    void start(uint8_t, bool) {}
    void stop() {}
    bool transfer(uint8_t, uint8_t*, size_t, uint8_t*, size_t) { return true; }
};

// PC implementation of SPI (stub)
class SPI_PC {
public:
    void init() {}
    uint32_t transfer(uint32_t) { return 0; }
    void transfer(void*, void*, size_t) {}
};

// PC implementation of Si5351 clock generator (stub)
class Si5351_PC {
public:
    void reset() {}
    bool init() { return true; }
    void disable_output() {}
    void enable_output() {}
    void set_clock(uint8_t, uint64_t, uint8_t) {}
};

// PC implementation of clock manager (stub)
class ClockManager_PC {
public:
    void init() {}
    void shutdown() {}
    void set_sampling_frequency(uint32_t) {}
    uint32_t sampling_frequency() const { return 3072000; }
};

// PC implementation of temperature logger (stub)
class TemperatureLogger_PC {
public:
    void feed() {}
    float temperature() const { return 25.0f; }
    bool is_stable() const { return true; }
};

// PC backlight implementation
class Backlight_PC : public Backlight {
public:
    void on() override { level_ = 255; }
    void off() override { level_ = 0; }
    void set_level(uint8_t l) override { level_ = l; }
    uint8_t level() const override { return level_; }
private:
    uint8_t level_ = 255;
};

// PC USB serial implementation (stub)
class USBSerial_PC {
public:
    void init() {}
    void shutdown() {}
    bool connected() const { return false; }
    size_t write(const void*, size_t) { return 0; }
    size_t read(void*, size_t) { return 0; }
};

// PC display adapter that wraps shim layer
class Display_PC {
public:
    static constexpr int width() { return 240; }
    static constexpr int height() { return 320; }

    void init() {}
    void shutdown() {}

    void fill_rectangle(int x, int y, int w, int h, uint16_t color) {
        shim::get_display().fill_rectangle(x, y, w, h, color);
    }

    void draw_pixel(int x, int y, uint16_t color) {
        shim::get_display().draw_pixel(x, y, color);
    }

    void render_line(int x, int y, int count, const uint16_t* colors) {
        for (int i = 0; i < count; ++i) {
            shim::get_display().draw_pixel(x + i, y, colors[i]);
        }
    }

    void scroll_set_area(uint16_t, uint16_t) {}
    void scroll(int16_t) {}
    void sleep() {}
    void wake() {}

    void render() {
        shim::get_display().present();
    }
};

// Global instances (these match what portapack.hpp declares as extern)
static IO_PC io_pc_instance;
static I2C_PC i2c0_pc_instance;
static SPI_PC ssp1_pc_instance;
static Si5351_PC clock_generator_pc_instance;
static ClockManager_PC clock_manager_pc_instance;
static TemperatureLogger_PC temperature_logger_pc_instance;
static Backlight_PC backlight_pc_instance;
static USBSerial_PC usb_serial_pc_instance;
static Display_PC display_pc_instance;

// Error message storage
const char* init_error = nullptr;

// State flags
uint32_t bl_tick_counter = 0;
bool antenna_bias = false;
uint16_t touch_threshold = 1800;
bool async_tx_enabled = false;

// Backlight accessor - returns pointer to backlight instance
Backlight* backlight() {
    return &backlight_pc_instance;
}

void set_antenna_bias(const bool v) {
    antenna_bias = v;
}

bool get_antenna_bias() {
    return antenna_bias;
}

// This function takes the hardware EventDispatcher which we don't use on PC
void setEventDispatcherToUSBSerial(EventDispatcher_HW*) {
    // No-op for PC emulator
}

init_status_t init() {
    std::cout << "portapack::init() - PC emulator mode" << std::endl;

    // The actual HAL initialization is done by portapack_shim::init()
    // which should be called before this

    if (!portapack_shim::is_initialized()) {
        init_error = "Shim layer not initialized";
        return init_status_t::INIT_NO_PORTAPACK;
    }

    return init_status_t::INIT_SUCCESS;
}

void shutdown(const bool leave_screen_on) {
    std::cout << "portapack::shutdown() - PC emulator mode" << std::endl;
    (void)leave_screen_on;
    // Actual shutdown handled by portapack_shim::shutdown()
}

} // namespace portapack

// Provide compatibility macros/types that the application code expects
// These bridge to our shim implementations

namespace lcd {
// Stub ILI9341 that wraps Display_PC
class ILI9341 {
public:
    static constexpr int width = 240;
    static constexpr int height = 320;

    void init() { portapack::display_pc_instance.init(); }
    void shutdown() { portapack::display_pc_instance.shutdown(); }

    void fill_rectangle(int x, int y, int w, int h, uint16_t color) {
        portapack::display_pc_instance.fill_rectangle(x, y, w, h, color);
    }

    void render() { portapack::display_pc_instance.render(); }
};
}

#endif // PORTAPACK_PC_EMULATOR
