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

#ifndef __PORTAPACK_SHIM_HPP__
#define __PORTAPACK_SHIM_HPP__

/**
 * @file portapack_shim.hpp
 * @brief Main header for the PortaPack PC Emulator shim layer.
 *
 * This header provides the central configuration and initialization
 * for the PC emulator, bridging the original PortaPack/Mayhem APIs
 * to the PC HAL implementations.
 *
 * The shim layer is enabled when PORTAPACK_PC_EMULATOR is defined.
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "../hal/pc/display_sdl.hpp"
#include "../hal/pc/audio_portaudio.hpp"
#include "../hal/pc/rf_soapysdr.hpp"
#include "../hal/pc/input_sdl.hpp"
#include "../hal/pc/storage_filesystem.hpp"
#include "../hal/pc/time_system.hpp"
#include "../hal/pc/baseband_emulator.hpp"

namespace portapack_shim {

/**
 * @brief Global HAL instances for PC emulation.
 *
 * These are the actual implementations that back the shim layer.
 * They are initialized in main_pc.cpp before the Mayhem UI starts.
 */
extern hal::DisplaySDL display_hal;
extern hal::AudioPortAudio audio_hal;
extern hal::RFMock rf_hal;  // Use mock by default, can be RFSoapySDR if hardware available
extern hal::InputSDL input_hal;
extern hal::StorageFilesystem storage_hal;
extern hal::TimeSystem time_hal;
extern baseband_emu::BasebandEmulator baseband_emu;

/**
 * @brief Initialize all PC HAL components.
 * @param sdcard_path Path to the emulated SD card directory
 * @param use_real_sdr If true, try to use real SDR hardware via SoapySDR
 * @return true if all components initialized successfully
 */
bool init(const std::string& sdcard_path = "./sdcard", bool use_real_sdr = false);

/**
 * @brief Shutdown all PC HAL components.
 */
void shutdown();

/**
 * @brief Check if the emulator is initialized.
 * @return true if initialized
 */
bool is_initialized();

/**
 * @brief Get the SDL window for event handling.
 * @return Pointer to SDL window, or nullptr if not initialized
 */
SDL_Window* get_window();

} // namespace portapack_shim

#endif // PORTAPACK_PC_EMULATOR

#endif // __PORTAPACK_SHIM_HPP__
