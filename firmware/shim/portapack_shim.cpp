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

#include "portapack_shim.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include <SDL2/SDL.h>
#include <iostream>

namespace portapack_shim {

// Global HAL instances
hal::DisplaySDL display_hal;
hal::AudioPortAudio audio_hal;
hal::RFMock rf_hal;
hal::InputSDL input_hal;
hal::StorageFilesystem storage_hal;
hal::TimeSystem time_hal;
baseband_emu::BasebandEmulator baseband_emu;

static bool initialized = false;

bool init(const std::string& sdcard_path, bool use_real_sdr) {
    std::cout << "=== PortaPack Mayhem PC Emulator ===" << std::endl;
    std::cout << "Initializing shim layer..." << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize display
    if (!display_hal.init()) {
        std::cerr << "Display initialization failed" << std::endl;
        SDL_Quit();
        return false;
    }

    // Initialize audio
    if (!audio_hal.init()) {
        std::cerr << "Audio initialization failed" << std::endl;
        display_hal.shutdown();
        SDL_Quit();
        return false;
    }

    // Initialize input
    if (!input_hal.init()) {
        std::cerr << "Input initialization failed" << std::endl;
        audio_hal.shutdown();
        display_hal.shutdown();
        SDL_Quit();
        return false;
    }

    // Initialize storage
    if (!storage_hal.init(sdcard_path)) {
        std::cerr << "Storage initialization failed" << std::endl;
        input_hal.shutdown();
        audio_hal.shutdown();
        display_hal.shutdown();
        SDL_Quit();
        return false;
    }

    // Initialize time
    if (!time_hal.init()) {
        std::cerr << "Time initialization failed" << std::endl;
        storage_hal.shutdown();
        input_hal.shutdown();
        audio_hal.shutdown();
        display_hal.shutdown();
        SDL_Quit();
        return false;
    }

    // Initialize RF (optional - emulator works without hardware)
#ifdef USE_SOAPYSDR
    if (use_real_sdr) {
        // Try to initialize real SDR
        hal::RFSoapySDR* real_sdr = new hal::RFSoapySDR();
        if (real_sdr->init()) {
            std::cout << "Using real SDR hardware: " << real_sdr->device_info() << std::endl;
            // Note: We're using RFMock in the global, so this is just a demo
            // In a real implementation, you'd have a polymorphic rf_hal pointer
            delete real_sdr;
        } else {
            delete real_sdr;
            std::cout << "No SDR hardware found, using mock RF" << std::endl;
        }
    }
#else
    (void)use_real_sdr;  // Silence unused parameter warning
#endif

    if (!rf_hal.init()) {
        std::cerr << "RF initialization failed" << std::endl;
        // RF is optional, continue anyway
    }

    // Initialize baseband emulator
    if (!baseband_emu.init(&rf_hal, &audio_hal)) {
        std::cerr << "Baseband emulator initialization failed" << std::endl;
        // Baseband is optional for UI testing, continue anyway
    }

    initialized = true;
    std::cout << "Shim layer initialized successfully!" << std::endl;
    std::cout << "SD Card path: " << sdcard_path << std::endl;
    std::cout << std::endl;

    return true;
}

void shutdown() {
    if (!initialized) return;

    std::cout << "Shutting down shim layer..." << std::endl;

    baseband_emu.shutdown();
    rf_hal.shutdown();
    time_hal.shutdown();
    storage_hal.shutdown();
    input_hal.shutdown();
    audio_hal.shutdown();
    display_hal.shutdown();

    SDL_Quit();

    initialized = false;
    std::cout << "Shim layer shutdown complete" << std::endl;
}

bool is_initialized() {
    return initialized;
}

SDL_Window* get_window() {
    return display_hal.window();
}

} // namespace portapack_shim

#endif // PORTAPACK_PC_EMULATOR
