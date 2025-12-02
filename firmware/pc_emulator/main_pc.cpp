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
 * @file main_pc.cpp
 * @brief Main entry point for the PortaPack Mayhem PC Emulator.
 *
 * This file initializes the shim layer and runs the main event loop,
 * providing a PC-based emulation environment for PortaPack Mayhem.
 */

#include "portapack_shim.hpp"
#include "event_shim.hpp"
#include "display_shim.hpp"
#include "audio_shim.hpp"
#include "receiver_model_shim.hpp"
#include "transmitter_model_shim.hpp"
#include "baseband_shim.hpp"
#include "shared_memory_shim.hpp"

#include <iostream>
#include <string>
#include <csignal>
#include <cstdlib>
#include <filesystem>

namespace {

// Global flag for signal handling
volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    (void)signum;
    g_running = 0;
    std::cout << "\nShutdown requested..." << std::endl;
}

void print_usage(const char* program_name) {
    std::cout << "PortaPack Mayhem PC Emulator v1.0.0\n"
              << "\n"
              << "Usage: " << program_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help           Show this help message\n"
              << "  -s, --sdcard PATH    Path to SD card directory (default: ./sdcard)\n"
              << "  -r, --real-sdr       Use real SDR hardware via SoapySDR\n"
              << "  -v, --verbose        Enable verbose logging\n"
              << "  -f, --frequency HZ   Initial center frequency in Hz\n"
              << "\n"
              << "Key Bindings:\n"
              << "  Arrow Keys           Navigation (Up/Down/Left/Right)\n"
              << "  Enter/Space          Select\n"
              << "  Escape/Backspace     Back\n"
              << "  Mouse Wheel          Encoder rotation\n"
              << "  Mouse Click          Touch input\n"
              << "  D                    DFU mode key\n"
              << "\n"
              << "Example:\n"
              << "  " << program_name << " -s ~/portapack_sd -r\n"
              << std::endl;
}

struct EmulatorConfig {
    std::string sdcard_path = "./sdcard";
    bool use_real_sdr = false;
    bool verbose = false;
    int64_t initial_frequency = 100000000;  // 100 MHz
};

bool parse_arguments(int argc, char* argv[], EmulatorConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return false;
        }
        else if (arg == "-s" || arg == "--sdcard") {
            if (i + 1 < argc) {
                config.sdcard_path = argv[++i];
            } else {
                std::cerr << "Error: --sdcard requires a path argument\n";
                return false;
            }
        }
        else if (arg == "-r" || arg == "--real-sdr") {
            config.use_real_sdr = true;
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "-f" || arg == "--frequency") {
            if (i + 1 < argc) {
                try {
                    config.initial_frequency = std::stoll(argv[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid frequency value\n";
                    return false;
                }
            } else {
                std::cerr << "Error: --frequency requires a value\n";
                return false;
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

bool ensure_sdcard_structure(const std::string& sdcard_path) {
    namespace fs = std::filesystem;

    try {
        // Create main SD card directory if it doesn't exist
        if (!fs::exists(sdcard_path)) {
            fs::create_directories(sdcard_path);
            std::cout << "Created SD card directory: " << sdcard_path << std::endl;
        }

        // Create standard PortaPack directories
        const char* subdirs[] = {
            "APPS",
            "ADS-B",
            "ADSB",
            "APRS",
            "AUDIO",
            "CAPTURES",
            "DEBUG",
            "FIRMWARE",
            "FREQMAN",
            "GPS",
            "LOGS",
            "LOOKINGGLASS",
            "PLAYLIST",
            "REMOTES",
            "SCREENSHOTS",
            "SETTINGS",
            "SIGNALS",
            "SPLASH",
            "WAV"
        };

        for (const auto& subdir : subdirs) {
            fs::path dir_path = fs::path(sdcard_path) / subdir;
            if (!fs::exists(dir_path)) {
                fs::create_directories(dir_path);
            }
        }

        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to create SD card structure: " << e.what() << std::endl;
        return false;
    }
}

void draw_startup_screen() {
    auto& display = shim::get_display();

    // Clear screen to dark blue
    display.fill_rectangle(0, 0, 240, 320, shim::Color(0, 0, 64));

    // Draw title area with gradient effect
    for (int y = 0; y < 60; ++y) {
        uint8_t intensity = static_cast<uint8_t>(64 + y);
        display.fill_rectangle(0, y, 240, 1, shim::Color(0, 0, intensity));
    }

    // Draw decorative lines
    display.fill_rectangle(0, 60, 240, 2, shim::Color(255, 255, 0));
    display.fill_rectangle(0, 62, 240, 1, shim::Color(128, 128, 0));

    // Status area background
    display.fill_rectangle(0, 280, 240, 40, shim::Color(32, 32, 32));
    display.fill_rectangle(0, 278, 240, 2, shim::Color(64, 64, 64));

    // Present the frame
    display.present();
}

void run_demo_animation(shim::EventDispatcher& dispatcher) {
    int frame = 0;
    auto& display = shim::get_display();

    dispatcher.set_tick_callback([&]() {
        frame++;

        // Animate a simple pattern
        int center_x = 120;
        int center_y = 170;
        int radius = 50 + (frame % 30);

        // Clear animation area
        display.fill_rectangle(60, 110, 120, 120, shim::Color(0, 0, 64));

        // Draw animated circle using rectangles
        for (int angle = 0; angle < 360; angle += 15) {
            double rad = angle * 3.14159265 / 180.0;
            int x = center_x + static_cast<int>(radius * 0.5 * std::cos(rad));
            int y = center_y + static_cast<int>(radius * 0.5 * std::sin(rad));

            uint8_t r = static_cast<uint8_t>((angle + frame * 3) % 256);
            uint8_t g = static_cast<uint8_t>((angle * 2 + frame) % 256);
            uint8_t b = static_cast<uint8_t>(255 - (angle + frame * 2) % 256);

            display.fill_rectangle(x - 3, y - 3, 6, 6, shim::Color(r, g, b));
        }

        // Draw frequency display simulation
        int64_t freq = shim::get_receiver_model().target_frequency();
        int freq_mhz = static_cast<int>(freq / 1000000);
        int freq_khz = static_cast<int>((freq % 1000000) / 1000);

        // Clear frequency area
        display.fill_rectangle(10, 250, 220, 25, shim::Color(0, 0, 32));

        // Draw frequency box
        display.fill_rectangle(10, 250, 220, 2, shim::Color(64, 64, 128));
        display.fill_rectangle(10, 273, 220, 2, shim::Color(64, 64, 128));
        display.fill_rectangle(10, 250, 2, 25, shim::Color(64, 64, 128));
        display.fill_rectangle(228, 250, 2, 25, shim::Color(64, 64, 128));
    });

    dispatcher.set_key_callback([&](shim::KeyEvent key, bool pressed) {
        if (!pressed) return;

        auto& rx = shim::get_receiver_model();
        int64_t freq = rx.target_frequency();
        int64_t step = rx.frequency_step();

        switch (key) {
            case shim::KeyEvent::Up:
                rx.set_target_frequency(freq + step * 10);
                break;
            case shim::KeyEvent::Down:
                rx.set_target_frequency(freq - step * 10);
                break;
            case shim::KeyEvent::Left:
                rx.set_target_frequency(freq - step);
                break;
            case shim::KeyEvent::Right:
                rx.set_target_frequency(freq + step);
                break;
            case shim::KeyEvent::Select:
                // Toggle RF amp
                rx.set_rf_amp(!rx.rf_amp());
                std::cout << "RF Amp: " << (rx.rf_amp() ? "ON" : "OFF") << std::endl;
                break;
            case shim::KeyEvent::Back:
                dispatcher.request_stop();
                break;
            default:
                break;
        }

        std::cout << "Frequency: " << (rx.target_frequency() / 1000000.0) << " MHz" << std::endl;
    });

    dispatcher.set_encoder_callback([&](int32_t delta) {
        auto& rx = shim::get_receiver_model();
        int64_t freq = rx.target_frequency();
        int64_t step = rx.frequency_step();
        rx.set_target_frequency(freq + delta * step);
        std::cout << "Frequency: " << (rx.target_frequency() / 1000000.0) << " MHz" << std::endl;
    });

    dispatcher.set_touch_callback([&](const shim::TouchEvent& event) {
        if (event.type == shim::TouchEvent::Type::Start) {
            std::cout << "Touch at (" << event.x << ", " << event.y << ")" << std::endl;

            // Draw a marker at touch position
            display.fill_rectangle(event.x - 5, event.y - 5, 10, 10, shim::Color(255, 0, 0));
        }
    });
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    std::cout << "========================================\n"
              << "  PortaPack Mayhem PC Emulator v1.0.0\n"
              << "========================================\n"
              << std::endl;

    // Parse command line arguments
    EmulatorConfig config;
    if (!parse_arguments(argc, argv, config)) {
        return 1;
    }

    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Ensure SD card directory structure exists
    if (!ensure_sdcard_structure(config.sdcard_path)) {
        std::cerr << "Failed to set up SD card directory" << std::endl;
        return 1;
    }

    std::cout << "SD Card Path: " << config.sdcard_path << std::endl;
    std::cout << "SDR Mode: " << (config.use_real_sdr ? "Real Hardware" : "Mock/Simulated") << std::endl;
    std::cout << "Initial Frequency: " << (config.initial_frequency / 1000000.0) << " MHz" << std::endl;
    std::cout << std::endl;

    // Initialize the shim layer
    std::cout << "Initializing emulator..." << std::endl;
    if (!portapack_shim::init(config.sdcard_path, config.use_real_sdr)) {
        std::cerr << "Failed to initialize PortaPack shim layer" << std::endl;
        return 1;
    }

    std::cout << "Emulator initialized successfully!" << std::endl;
    std::cout << std::endl;

    // Set initial frequency
    shim::get_receiver_model().set_target_frequency(config.initial_frequency);

    // Draw startup screen
    draw_startup_screen();

    // Get the event dispatcher
    auto& dispatcher = shim::get_event_dispatcher();

    // Set up demo callbacks
    run_demo_animation(dispatcher);

    std::cout << "Starting event loop...\n"
              << "Press ESC or close window to exit.\n"
              << std::endl;

    // Run the main event loop
    // This blocks until stop is requested
    while (g_running && !dispatcher.should_stop()) {
        dispatcher.run();
    }

    std::cout << "\nShutting down emulator..." << std::endl;

    // Cleanup
    portapack_shim::shutdown();

    std::cout << "Emulator shutdown complete." << std::endl;

    return 0;
}
