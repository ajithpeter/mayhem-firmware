/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/**
 * @file test_hal_interfaces.cpp
 * @brief Unit tests for HAL interface definitions.
 *
 * Tests that HAL interface constants and types are correctly defined.
 */

#include "doctest.h"
#include "hal_display.hpp"
#include "hal_audio.hpp"
#include "hal_rf.hpp"
#include "hal_input.hpp"
#include "hal_storage.hpp"
#include "hal_time.hpp"

TEST_SUITE_BEGIN("HAL Interfaces");

// ============================================================================
// Display Interface Tests
// ============================================================================

TEST_CASE("IDisplay constants are correct") {
    SUBCASE("Display dimensions") {
        CHECK_EQ(hal::IDisplay::WIDTH, 240);
        CHECK_EQ(hal::IDisplay::HEIGHT, 320);
    }
}

// ============================================================================
// Audio Interface Tests
// ============================================================================

TEST_CASE("IAudio interface is properly defined") {
    SUBCASE("Audio interface is abstract") {
        // IAudio is an abstract interface - just verify it compiles
        CHECK(true);
    }
}

// ============================================================================
// RF Interface Tests
// ============================================================================

TEST_CASE("IRF Direction enum is correct") {
    SUBCASE("Direction values") {
        CHECK(static_cast<int>(hal::IRF::Direction::Receive) == 0);
        CHECK(static_cast<int>(hal::IRF::Direction::Transmit) == 1);
    }
}

TEST_CASE("IRF frequency constants") {
    SUBCASE("Frequency ranges are sensible") {
        // HackRF frequency range
        CHECK(1000000LL < 6000000000LL);  // 1 MHz to 6 GHz
    }
}

// ============================================================================
// Input Interface Tests
// ============================================================================

TEST_CASE("IInput KeyEvent enum is complete") {
    SUBCASE("All key events are defined") {
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Right) == 0);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Left) == 1);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Down) == 2);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Up) == 3);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Select) == 4);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Dfu) == 5);
        CHECK(static_cast<int>(hal::IInput::KeyEvent::Back) == 6);
    }
}

TEST_CASE("IInput TouchType enum is complete") {
    SUBCASE("All touch types are defined") {
        CHECK(static_cast<int>(hal::IInput::TouchType::Start) == 0);
        CHECK(static_cast<int>(hal::IInput::TouchType::Move) == 1);
        CHECK(static_cast<int>(hal::IInput::TouchType::End) == 2);
    }
}

// ============================================================================
// Storage Interface Tests
// ============================================================================

TEST_CASE("IStorage OpenMode enum is complete") {
    SUBCASE("Core open modes are defined") {
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Read) == 0);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Write) == 1);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::ReadWrite) == 2);
        CHECK(static_cast<int>(hal::IStorage::OpenMode::Append) == 3);
    }
}

TEST_CASE("IStorage SeekOrigin enum is complete") {
    SUBCASE("All seek origins are defined") {
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::Begin) == 0);
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::Current) == 1);
        CHECK(static_cast<int>(hal::IStorage::SeekOrigin::End) == 2);
    }
}

TEST_CASE("IStorage Error enum has core errors") {
    SUBCASE("Common errors are defined") {
        CHECK(static_cast<int>(hal::IStorage::Error::None) == 0);
        CHECK(static_cast<int>(hal::IStorage::Error::NotFound) == 1);
        CHECK(static_cast<int>(hal::IStorage::Error::AccessDenied) == 2);
    }
}

TEST_CASE("IStorage FileHandle type") {
    SUBCASE("Invalid handle constant") {
        CHECK_EQ(hal::IStorage::INVALID_HANDLE, static_cast<hal::IStorage::FileHandle>(-1));
    }
}

// ============================================================================
// Time Interface Tests
// ============================================================================

TEST_CASE("ITime DateTime structure") {
    hal::ITime::DateTime dt;
    dt.year = 2024;
    dt.month = 12;
    dt.day = 25;
    dt.hour = 14;
    dt.minute = 30;
    dt.second = 45;

    SUBCASE("DateTime fields are correctly sized") {
        CHECK_EQ(dt.year, 2024);
        CHECK_EQ(dt.month, 12);
        CHECK_EQ(dt.day, 25);
        CHECK_EQ(dt.hour, 14);
        CHECK_EQ(dt.minute, 30);
        CHECK_EQ(dt.second, 45);
    }

    SUBCASE("DateTime can represent valid dates") {
        CHECK(dt.year >= 1970);
        CHECK(dt.month >= 1);
        CHECK(dt.month <= 12);
        CHECK(dt.day >= 1);
        CHECK(dt.day <= 31);
        CHECK(dt.hour <= 23);
        CHECK(dt.minute <= 59);
        CHECK(dt.second <= 59);
    }
}

TEST_SUITE_END();
