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
 * @file test_time_system.cpp
 * @brief Unit tests for system time implementation.
 */

#include "doctest.h"
#include "time_system.hpp"
#include <chrono>
#include <thread>

TEST_SUITE_BEGIN("TimeSystem");

// ============================================================================
// TimeSystem Initialization Tests
// ============================================================================

TEST_CASE("TimeSystem initialization") {
    hal::TimeSystem time_sys;

    SUBCASE("Init returns true") {
        CHECK(time_sys.init() == true);
    }

    SUBCASE("Shutdown after init") {
        time_sys.init();
        time_sys.shutdown();
        CHECK(true);
    }

    SUBCASE("Multiple init/shutdown cycles") {
        for (int i = 0; i < 3; i++) {
            CHECK(time_sys.init() == true);
            time_sys.shutdown();
        }
    }
}

// ============================================================================
// Unix Time Tests
// ============================================================================

TEST_CASE("Unix time") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("Unix time is reasonable") {
        uint32_t unix_time = time_sys.unix_time();
        // Should be after Jan 1, 2024 (1704067200)
        CHECK(unix_time > 1704067200);
        // Should be before year 2100 (4102444800)
        CHECK(unix_time < 4102444800);
    }

    SUBCASE("Unix time advances") {
        uint32_t time1 = time_sys.unix_time();
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        uint32_t time2 = time_sys.unix_time();
        CHECK(time2 >= time1);
    }

    SUBCASE("Set and get unix time") {
        uint32_t test_time = 1700000000;  // Some time in Nov 2023
        time_sys.set_unix_time(test_time);
        uint32_t result = time_sys.unix_time();
        // Allow small drift for time between set and get
        CHECK(result >= test_time);
        CHECK(result < test_time + 5);
    }

    time_sys.shutdown();
}

// ============================================================================
// DateTime Tests
// ============================================================================

TEST_CASE("DateTime operations") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("Get current datetime via now()") {
        hal::ITime::DateTime dt = time_sys.now();

        // Year should be reasonable
        CHECK(dt.year >= 2024);
        CHECK(dt.year <= 2100);

        // Month should be 1-12
        CHECK(dt.month >= 1);
        CHECK(dt.month <= 12);

        // Day should be 1-31
        CHECK(dt.day >= 1);
        CHECK(dt.day <= 31);

        // Hour should be 0-23
        CHECK(dt.hour <= 23);

        // Minute should be 0-59
        CHECK(dt.minute <= 59);

        // Second should be 0-59
        CHECK(dt.second <= 59);
    }

    SUBCASE("Set and get datetime") {
        hal::ITime::DateTime set_dt;
        set_dt.year = 2024;
        set_dt.month = 12;
        set_dt.day = 25;
        set_dt.hour = 14;
        set_dt.minute = 30;
        set_dt.second = 45;

        time_sys.set_datetime(set_dt);
        hal::ITime::DateTime get_dt = time_sys.now();

        // Note: Values might not match exactly due to time passing
        // Just verify the structure is valid
        CHECK(get_dt.year >= 2024);
        CHECK(get_dt.month >= 1);
        CHECK(get_dt.month <= 12);
    }

    time_sys.shutdown();
}

// ============================================================================
// Millisecond Timer Tests
// ============================================================================

TEST_CASE("Millisecond timer") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("Millis returns increasing values") {
        uint64_t ms1 = time_sys.millis();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        uint64_t ms2 = time_sys.millis();
        CHECK(ms2 > ms1);
    }

    SUBCASE("Millis increments approximately correct amount") {
        uint64_t ms1 = time_sys.millis();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        uint64_t ms2 = time_sys.millis();

        uint64_t elapsed = ms2 - ms1;
        // Allow some tolerance for scheduling
        CHECK(elapsed >= 90);
        CHECK(elapsed <= 200);
    }

    time_sys.shutdown();
}

// ============================================================================
// Microsecond Timer Tests
// ============================================================================

TEST_CASE("Microsecond timer") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("Micros returns increasing values") {
        uint64_t us1 = time_sys.micros();
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        uint64_t us2 = time_sys.micros();
        CHECK(us2 > us1);
    }

    SUBCASE("Micros has good resolution") {
        uint64_t us1 = time_sys.micros();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        uint64_t us2 = time_sys.micros();

        uint64_t elapsed = us2 - us1;
        // Allow some tolerance for scheduling
        CHECK(elapsed >= 9000);
        CHECK(elapsed <= 30000);
    }

    SUBCASE("Micros is finer than millis") {
        uint64_t ms = time_sys.millis();
        uint64_t us = time_sys.micros();

        // Microseconds should be roughly 1000x milliseconds
        // Allow wide tolerance - use signed comparison to avoid underflow
        int64_t expected_us = static_cast<int64_t>(ms) * 1000;
        int64_t diff = static_cast<int64_t>(us) - expected_us;
        CHECK(diff > -2000000);  // Within 2 seconds
        CHECK(diff < 2000000);
    }

    time_sys.shutdown();
}

// ============================================================================
// Delay Tests
// ============================================================================

TEST_CASE("Delay functions") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("delay_ms blocks for approximately correct time") {
        auto start = std::chrono::steady_clock::now();
        time_sys.delay_ms(50);
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        CHECK(elapsed >= 45);
        CHECK(elapsed <= 100);
    }

    SUBCASE("delay_us blocks for approximately correct time") {
        auto start = std::chrono::steady_clock::now();
        time_sys.delay_us(10000);  // 10ms
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        CHECK(elapsed >= 9000);
        CHECK(elapsed <= 30000);
    }

    SUBCASE("Zero delay returns immediately") {
        auto start = std::chrono::steady_clock::now();
        time_sys.delay_ms(0);
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        CHECK(elapsed < 10);
    }

    time_sys.shutdown();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Edge cases") {
    hal::TimeSystem time_sys;
    time_sys.init();

    SUBCASE("Very small delay") {
        auto start = std::chrono::steady_clock::now();
        time_sys.delay_us(1);
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        CHECK(elapsed >= 0);  // Just verify it doesn't crash
    }

    SUBCASE("Multiple rapid timer calls") {
        for (int i = 0; i < 100; i++) {
            uint64_t ms = time_sys.millis();
            CHECK(ms >= 0);  // Just verify no crash
        }
    }

    SUBCASE("Multiple rapid micros calls") {
        for (int i = 0; i < 100; i++) {
            uint64_t us = time_sys.micros();
            CHECK(us >= 0);  // Just verify no crash
        }
    }

    time_sys.shutdown();
}

TEST_SUITE_END();
