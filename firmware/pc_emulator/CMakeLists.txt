# Copyright (C) 2024 Mayhem PC Emulator Project
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

cmake_minimum_required(VERSION 3.16)
project(portapack_emulator VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Build Options
# ============================================================================

option(USE_SOAPYSDR "Enable SoapySDR for real SDR hardware support" ON)
option(USE_MOCK_RF "Use mock RF backend instead of SoapySDR" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# ============================================================================
# C++ Standard and Compiler Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-function
    )
endif()

# Debug/Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions(-DDEBUG)
else()
    add_compile_options(-O2)
    add_definitions(-DNDEBUG)
endif()

# ============================================================================
# PC Emulator Definition
# ============================================================================

add_definitions(-DPORTAPACK_PC_EMULATOR=1)

# ============================================================================
# Find Required Packages
# ============================================================================

# SDL2
find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    message(STATUS "Found SDL2: ${SDL2_VERSION}")
else()
    message(FATAL_ERROR "SDL2 not found. Please install libsdl2-dev")
endif()

# PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
if(PORTAUDIO_FOUND)
    message(STATUS "Found PortAudio: ${PORTAUDIO_VERSION}")
else()
    message(FATAL_ERROR "PortAudio not found. Please install libportaudio-dev")
endif()

# SoapySDR (optional)
if(USE_SOAPYSDR AND NOT USE_MOCK_RF)
    find_package(SoapySDR CONFIG)
    if(SoapySDR_FOUND)
        message(STATUS "Found SoapySDR: ${SoapySDR_VERSION}")
        add_definitions(-DUSE_SOAPYSDR=1)
    else()
        message(WARNING "SoapySDR not found. Using mock RF backend.")
        set(USE_MOCK_RF ON)
    endif()
endif()

if(USE_MOCK_RF)
    add_definitions(-DUSE_MOCK_RF=1)
    message(STATUS "Using mock RF backend (no real SDR hardware)")
endif()

# ============================================================================
# Source Files
# ============================================================================

set(HAL_INTERFACE_HEADERS
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_display.hpp
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_audio.hpp
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_rf.hpp
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_input.hpp
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_storage.hpp
    ${CMAKE_SOURCE_DIR}/../hal/interface/hal_time.hpp
)

set(HAL_PC_SOURCES
    ${CMAKE_SOURCE_DIR}/../hal/pc/display_sdl.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/audio_portaudio.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/rf_soapysdr.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/input_sdl.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/storage_filesystem.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/time_system.cpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/baseband_emulator.cpp
)

set(HAL_PC_HEADERS
    ${CMAKE_SOURCE_DIR}/../hal/pc/display_sdl.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/audio_portaudio.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/rf_soapysdr.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/input_sdl.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/storage_filesystem.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/time_system.hpp
    ${CMAKE_SOURCE_DIR}/../hal/pc/baseband_emulator.hpp
)

set(SHIM_SOURCES
    ${CMAKE_SOURCE_DIR}/../shim/portapack_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/shared_memory_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/display_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/audio_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/baseband_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/receiver_model_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/transmitter_model_shim.cpp
    ${CMAKE_SOURCE_DIR}/../shim/event_shim.cpp
)

set(SHIM_HEADERS
    ${CMAKE_SOURCE_DIR}/../shim/portapack_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/shared_memory_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/display_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/audio_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/baseband_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/receiver_model_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/transmitter_model_shim.hpp
    ${CMAKE_SOURCE_DIR}/../shim/event_shim.hpp
)

set(EMULATOR_SOURCES
    ${CMAKE_SOURCE_DIR}/main_pc.cpp
)

# ============================================================================
# Create Executable
# ============================================================================

add_executable(portapack_emulator
    ${EMULATOR_SOURCES}
    ${HAL_PC_SOURCES}
    ${SHIM_SOURCES}
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(portapack_emulator PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../hal/interface
    ${CMAKE_SOURCE_DIR}/../hal/pc
    ${CMAKE_SOURCE_DIR}/../shim
    ${SDL2_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
)

if(SoapySDR_FOUND)
    target_include_directories(portapack_emulator PRIVATE
        ${SoapySDR_INCLUDE_DIRS}
    )
endif()

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(portapack_emulator PRIVATE
    ${SDL2_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    pthread
)

if(SoapySDR_FOUND)
    target_link_libraries(portapack_emulator PRIVATE
        ${SoapySDR_LIBRARIES}
    )
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS portapack_emulator
    RUNTIME DESTINATION bin
)

# Create default SD card directory structure
install(CODE "
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/APPS)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/ADS-B)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/CAPTURES)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/DEBUG)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/FIRMWARE)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/FREQMAN)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/LOGS)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/LOOKINGGLASS)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/SCREENSHOTS)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/portapack_emulator/sdcard/SETTINGS)
")

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== PortaPack Mayhem PC Emulator Build Configuration ===")
message(STATUS "")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "  SDL2:           ${SDL2_VERSION}")
message(STATUS "  PortAudio:      ${PORTAUDIO_VERSION}")
if(SoapySDR_FOUND)
message(STATUS "  SoapySDR:       ${SoapySDR_VERSION}")
else()
message(STATUS "  SoapySDR:       Not found (using mock RF)")
endif()
message(STATUS "  Mock RF:        ${USE_MOCK_RF}")
message(STATUS "")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
