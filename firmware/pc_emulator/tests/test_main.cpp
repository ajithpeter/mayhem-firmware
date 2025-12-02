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
 * @file test_main.cpp
 * @brief Main entry point for PC emulator unit tests.
 *
 * Uses doctest framework - the same testing framework used by the main
 * PortaPack Mayhem firmware tests.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Basic sanity test
TEST_CASE("Test framework is working") {
    REQUIRE(1 == 1);
    REQUIRE_FALSE(1 == 2);
    CHECK(true);
}
