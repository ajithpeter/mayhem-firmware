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
 * PortaPack PC Emulator Header
 *
 * This provides PC-compatible implementations of PortaPack globals and
 * interfaces. It can be used as a drop-in replacement for portapack.hpp
 * when building for PC.
 */

#ifndef __PORTAPACK_PC_HPP__
#define __PORTAPACK_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <string>
#include <functional>

// Forward declarations
namespace lcd {
class ILI9341_PC;
}

namespace ui {
class ThemeTemplate;
class Theme;
}

namespace portapack {

/* ============================================================================
 * Initialization Status
 * ============================================================================ */

enum class init_status_t {
    INIT_SUCCESS,
    INIT_NO_PORTAPACK,
    INIT_PORTAPACK_CPLD_FAILED,
    INIT_HACKRF_CPLD_FAILED,
};

extern const char* init_error;

/* ============================================================================
 * Display Interface
 * ============================================================================ */

/**
 * @brief PC-compatible display interface that wraps SDL display.
 */
class DisplayPC {
public:
    static constexpr int width = 240;
    static constexpr int height = 320;

    void fill_rectangle(int x, int y, int w, int h, uint16_t color);
    void draw_pixel(int x, int y, uint16_t color);
    void draw_pixels(int x, int y, const uint16_t* pixels, int count);
    void scroll_set_area(int top, int bottom);
    void scroll_set_position(int position);
    void sleep();
    void wake();
    void present();
};

extern DisplayPC display;

/* ============================================================================
 * Receiver Model
 * ============================================================================ */

class ReceiverModelPC {
public:
    int64_t target_frequency() const { return frequency_; }
    void set_target_frequency(int64_t freq) { frequency_ = freq; }

    int64_t frequency_step() const { return frequency_step_; }
    void set_frequency_step(int64_t step) { frequency_step_ = step; }

    int32_t lna() const { return lna_gain_; }
    void set_lna(int32_t gain) { lna_gain_ = gain; }

    int32_t vga() const { return vga_gain_; }
    void set_vga(int32_t gain) { vga_gain_ = gain; }

    bool rf_amp() const { return rf_amp_; }
    void set_rf_amp(bool enabled) { rf_amp_ = enabled; }

    int32_t modulation() const { return modulation_; }
    void set_modulation(int32_t mod) { modulation_ = mod; }

    int32_t am_configuration() const { return am_config_; }
    void set_am_configuration(int32_t config) { am_config_ = config; }

    int32_t nbfm_configuration() const { return nbfm_config_; }
    void set_nbfm_configuration(int32_t config) { nbfm_config_ = config; }

    int32_t wfm_configuration() const { return wfm_config_; }
    void set_wfm_configuration(int32_t config) { wfm_config_ = config; }

    uint32_t baseband_bandwidth() const { return baseband_bandwidth_; }
    void set_baseband_bandwidth(uint32_t bw) { baseband_bandwidth_ = bw; }

    uint32_t sampling_rate() const { return sampling_rate_; }
    void set_sampling_rate(uint32_t rate) { sampling_rate_ = rate; }

    bool headphone_volume_enabled() const { return headphone_enabled_; }
    void set_headphone_volume_enabled(bool enabled) { headphone_enabled_ = enabled; }

    float normalized_headphone_volume() const { return headphone_volume_; }
    void set_normalized_headphone_volume(float vol) { headphone_volume_ = vol; }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }

private:
    int64_t frequency_ = 100000000;  // 100 MHz
    int64_t frequency_step_ = 25000;  // 25 kHz
    int32_t lna_gain_ = 32;
    int32_t vga_gain_ = 32;
    bool rf_amp_ = false;
    int32_t modulation_ = 0;
    int32_t am_config_ = 0;
    int32_t nbfm_config_ = 0;
    int32_t wfm_config_ = 0;
    uint32_t baseband_bandwidth_ = 1750000;
    uint32_t sampling_rate_ = 3072000;
    bool headphone_enabled_ = true;
    float headphone_volume_ = 0.5f;
    bool enabled_ = false;
};

extern ReceiverModelPC receiver_model;

/* ============================================================================
 * Transmitter Model
 * ============================================================================ */

class TransmitterModelPC {
public:
    int64_t target_frequency() const { return frequency_; }
    void set_target_frequency(int64_t freq) { frequency_ = freq; }

    int32_t tx_gain() const { return tx_gain_; }
    void set_tx_gain(int32_t gain) { tx_gain_ = gain; }

    bool rf_amp() const { return rf_amp_; }
    void set_rf_amp(bool enabled) { rf_amp_ = enabled; }

    uint32_t baseband_bandwidth() const { return baseband_bandwidth_; }
    void set_baseband_bandwidth(uint32_t bw) { baseband_bandwidth_ = bw; }

    uint32_t sampling_rate() const { return sampling_rate_; }
    void set_sampling_rate(uint32_t rate) { sampling_rate_ = rate; }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }

private:
    int64_t frequency_ = 433920000;  // 433.92 MHz
    int32_t tx_gain_ = 47;
    bool rf_amp_ = false;
    uint32_t baseband_bandwidth_ = 1750000;
    uint32_t sampling_rate_ = 3072000;
    bool enabled_ = false;
};

extern TransmitterModelPC transmitter_model;

/* ============================================================================
 * Global Functions
 * ============================================================================ */

extern uint32_t bl_tick_counter;
extern bool antenna_bias;
extern uint16_t touch_threshold;

void set_antenna_bias(bool v);
bool get_antenna_bias();

init_status_t init();
void shutdown();

/* ============================================================================
 * Persistent Memory (stub interface)
 * ============================================================================ */

namespace persistent_memory {

enum class dst_config_t {
    DST_OFF = 0,
    DST_AUTO_US = 1,
    DST_AUTO_EU = 2,
};

uint32_t config_cpld();
void set_config_cpld(uint32_t value);

int64_t tuned_frequency();
void set_tuned_frequency(int64_t freq);

int32_t afsk_mark_freq();
void set_afsk_mark_freq(int32_t freq);

int32_t afsk_space_freq();
void set_afsk_space_freq(int32_t freq);

int32_t modem_baudrate();
void set_modem_baudrate(int32_t rate);

int32_t modem_repeat();
void set_modem_repeat(int32_t repeat);

bool config_audio_mute();
void set_config_audio_mute(bool mute);

bool config_speaker_disable();
void set_config_speaker_disable(bool disable);

dst_config_t dst_config();
void set_dst_config(dst_config_t config);

} // namespace persistent_memory

} // namespace portapack

/* ============================================================================
 * RF Namespace
 * ============================================================================ */

namespace rf {

using Frequency = int64_t;

enum class Direction {
    Receive = 0,
    Transmit = 1,
};

template <typename T>
struct range_t {
    const T minimum;
    const T maximum;
};

using FrequencyRange = range_t<Frequency>;

namespace path {
constexpr FrequencyRange band_low{0, 2170000000LL};
constexpr FrequencyRange band_high{2740000000LL, 7250000000LL};
constexpr FrequencyRange band_mid{band_low.maximum, band_high.minimum};
}

constexpr FrequencyRange tuning_range{path::band_low.minimum, path::band_high.maximum};

} // namespace rf

/* ============================================================================
 * RTC Namespace
 * ============================================================================ */

namespace rtc {

struct RTC {
    uint32_t tv_date;  // (year << 16) | (month << 8) | day
    uint32_t tv_time;  // (hour << 16) | (minute << 8) | second

    constexpr RTC() : tv_date(0), tv_time(0) {}
    constexpr RTC(uint32_t year, uint32_t month, uint32_t day,
                  uint32_t hour, uint32_t minute, uint32_t second)
        : tv_date((year << 16) | (month << 8) | day),
          tv_time((hour << 16) | (minute << 8) | second) {}

    uint16_t year() const { return (tv_date >> 16) & 0xfff; }
    uint8_t month() const { return (tv_date >> 8) & 0x0f; }
    uint8_t day() const { return tv_date & 0x1f; }
    uint8_t hour() const { return (tv_time >> 16) & 0x1f; }
    uint8_t minute() const { return (tv_time >> 8) & 0x3f; }
    uint8_t second() const { return tv_time & 0x3f; }
};

} // namespace rtc

/* ============================================================================
 * RTC Time Namespace
 * ============================================================================ */

#include <functional>
#include <list>
#include <memory>
#include <ctime>

using SignalToken = uint32_t;

template <class... Args>
struct Signal {
    using Callback = std::function<void(Args...)>;

    SignalToken operator+=(const Callback& callback) {
        const SignalToken token = next_token++;
        entries.emplace_back(std::make_unique<CallbackEntry>(callback, token));
        return token;
    }

    bool operator-=(const SignalToken token) {
        entries.remove_if([token](auto& entry) {
            return entry->token == token;
        });
        return true;
    }

    void emit(Args... args) {
        for (auto& entry : entries) {
            entry->callback(args...);
        }
    }

private:
    struct CallbackEntry {
        const Callback callback;
        const SignalToken token;
        CallbackEntry(const Callback& cb, SignalToken t) : callback(cb), token(t) {}
    };
    std::list<std::unique_ptr<CallbackEntry>> entries;
    SignalToken next_token = 1;
};

namespace rtc_time {

extern Signal<> signal_tick_second;

void on_tick_second();
void set(rtc::RTC& new_datetime);
rtc::RTC now();
rtc::RTC now(rtc::RTC& out_datetime);

// Daylight savings time functions
void dst_init();
rtc::RTC dst_adjust_returned_time(rtc::RTC& datetime);

} // namespace rtc_time

#endif /* PORTAPACK_PC_EMULATOR */

#endif /* __PORTAPACK_PC_HPP__ */
