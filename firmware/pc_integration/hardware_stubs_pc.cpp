/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * PC-compatible stubs for hardware-dependent functions.
 * These replace ChibiOS/LPC43xx-specific implementations for PC emulation.
 */

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <ctime>
#include <bitset>

// Forward declarations to match firmware headers
namespace lpc43xx {
namespace rtc {
struct RTC {
    uint16_t year_;
    uint8_t month_;
    uint8_t day_;
    uint8_t hour_;
    uint8_t minute_;
    uint8_t second_;

    uint16_t year() const { return year_; }
    uint8_t month() const { return month_; }
    uint8_t day() const { return day_; }
    uint8_t hour() const { return hour_; }
    uint8_t minute() const { return minute_; }
    uint8_t second() const { return second_; }

    RTC() : year_(2024), month_(1), day_(1), hour_(0), minute_(0), second_(0) {}
    RTC(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s)
        : year_(y), month_(mo), day_(d), hour_(h), minute_(mi), second_(s) {}
};
} // namespace rtc
} // namespace lpc43xx

// Signal template stub
template<typename... Args>
class Signal {
public:
    template<typename F>
    Signal& operator+=(F&&) { return *this; }
    template<typename F>
    Signal& operator-=(F&&) { return *this; }
    void emit(Args...) {}
};

namespace rtc_time {

Signal<> signal_tick_second;

void on_tick_second() {
    signal_tick_second.emit();
}

lpc43xx::rtc::RTC now() {
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);
    return lpc43xx::rtc::RTC(
        tm->tm_year + 1900,
        tm->tm_mon + 1,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec
    );
}

lpc43xx::rtc::RTC now(lpc43xx::rtc::RTC& out_datetime) {
    out_datetime = now();
    return out_datetime;
}

void dst_init() {}

} // namespace rtc_time

// Switch/input stubs
enum class Switch : uint8_t {
    Right = 0,
    Left = 1,
    Down = 2,
    Up = 3,
    Sel = 4,
    Dfu = 5
};

using SwitchesState = std::bitset<6>;

static SwitchesState long_press_config{};

SwitchesState get_switches_long_press_config() {
    return long_press_config;
}

void set_switches_long_press_config(SwitchesState switch_config) {
    long_press_config = switch_config;
}

bool switch_is_long_pressed(Switch s) {
    (void)s;
    return false;
}

// Note: uint_to_char and char_to_uint are defined in string_format.cpp

#endif // PORTAPACK_PC_EMULATOR
