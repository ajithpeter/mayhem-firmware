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

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

// Forward declarations for types
namespace rf {
    using Frequency = int64_t;
}

namespace touch {
    struct Calibration {
        struct {
            int32_t a, b, c, d, e, f;
        } transform;
    };
}

namespace modems {
    struct serial_format_t {
        uint32_t data_bits;
        uint32_t parity;
        uint32_t stop_bits;
        bool bit_order_msb;
    };
}

namespace volume {
    struct volume_t {
        int32_t centibel;
    };
}

namespace ui {
    struct Color {
        uint16_t v;
        Color() : v(0) {}
        Color(uint16_t val) : v(val) {}
        Color(uint8_t r, uint8_t g, uint8_t b) {
            v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
    };
}

namespace portapack {
namespace persistent_memory {

// Static storage for settings
static struct {
    rf::Frequency target_frequency = 100000000;  // 100 MHz
    volume::volume_t headphone_volume{-300};     // -30 dB
    int32_t correction_ppb = 0;
    touch::Calibration touch_cal{};
    modems::serial_format_t serial_fmt{8, 0, 1, false};
    int32_t tone_mix = 0;
    int32_t afsk_mark = 1200;
    int32_t afsk_space = 2200;
    int32_t modem_baudrate = 1200;
    uint8_t modem_repeat = 5;
    bool stealth_mode = false;
    uint8_t config_cpld_val = 0;
    bool splash_enabled = true;
    bool converter_enabled = false;
    bool updown_converter = false;
    int64_t converter_freq = 0;
    bool gui_return_icon = true;
    bool clock_hidden = false;
    bool clock_with_date_enabled = false;
    bool login_enabled = false;
    bool audio_muted = false;
    bool speaker_disabled = false;
    bool touchscreen_disabled = false;
    bool load_app_settings_val = true;
    bool save_app_settings_val = true;
    uint8_t encoder_sensitivity = 0;
    uint8_t encoder_rate = 1;
    bool encoder_dial_dir = false;
    uint32_t pocsag_last_addr = 0;
    uint32_t pocsag_ignore_addr = 0;
    bool clkout_on = false;
    uint16_t clkout_frequency = 10000;
    bool dst_on = false;
    bool fake_brightness_on = false;
    uint8_t fake_brightness = 1;
    uint16_t touch_threshold = 1800;
    ui::Color menu_color_val{0, 128, 255};  // Default blue
    uint8_t theme_id = 0;
    bool hide_speaker = false;
    bool hide_mute = false;
    bool hide_converter = false;
    bool hide_stealth = false;
    bool hide_camera = false;
    bool hide_sleep = false;
    bool hide_bias_tee = false;
    bool hide_clock = false;
    bool hide_fake_brightness = false;
    bool hide_numeric_battery = false;
    bool hide_battery_icon = false;
    bool hide_sd_card = false;
} settings;

// Backlight configuration
enum backlight_timeout_t {
    Timeout5Sec = 0,
    Timeout15Sec = 1,
    Timeout30Sec = 2,
    Timeout60Sec = 3,
    Timeout180Sec = 4,
    Timeout300Sec = 5,
    Timeout600Sec = 6,
    Timeout3600Sec = 7,
};

struct backlight_config_t {
    backlight_timeout_t _timeout_enum = Timeout600Sec;
    bool _timeout_enabled = false;

    bool timeout_enabled() const { return _timeout_enabled; }
    backlight_timeout_t timeout_enum() const { return _timeout_enum; }
    uint32_t timeout_seconds() const {
        switch (_timeout_enum) {
            case Timeout5Sec: return 5;
            case Timeout15Sec: return 15;
            case Timeout30Sec: return 30;
            case Timeout60Sec: return 60;
            case Timeout180Sec: return 180;
            case Timeout300Sec: return 300;
            case Timeout3600Sec: return 3600;
            default: return 600;
        }
    }
};

static backlight_config_t backlight_cfg;

namespace cache {
    void defaults() {
        std::cout << "persistent_memory::cache::defaults()" << std::endl;
    }

    void init() {
        std::cout << "persistent_memory::cache::init() - PC emulator" << std::endl;
    }

    void persist() {
        // Would save to file in real implementation
    }
}

using ppb_t = int32_t;

rf::Frequency target_frequency() { return settings.target_frequency; }
void set_target_frequency(const rf::Frequency new_value) { settings.target_frequency = new_value; }

volume::volume_t headphone_volume() { return settings.headphone_volume; }
void set_headphone_volume(volume::volume_t new_value) { settings.headphone_volume = new_value; }

ppb_t correction_ppb() { return settings.correction_ppb; }
void set_correction_ppb(const ppb_t new_value) { settings.correction_ppb = new_value; }

void set_touch_calibration(const touch::Calibration& new_value) { settings.touch_cal = new_value; }
const touch::Calibration& touch_calibration() { return settings.touch_cal; }

modems::serial_format_t serial_format() { return settings.serial_fmt; }
void set_serial_format(const modems::serial_format_t new_value) { settings.serial_fmt = new_value; }

int32_t tone_mix() { return settings.tone_mix; }
void set_tone_mix(const int32_t new_value) { settings.tone_mix = new_value; }

int32_t afsk_mark_freq() { return settings.afsk_mark; }
void set_afsk_mark(const int32_t new_value) { settings.afsk_mark = new_value; }

int32_t afsk_space_freq() { return settings.afsk_space; }
void set_afsk_space(const int32_t new_value) { settings.afsk_space = new_value; }

uint32_t get_modem_def_index() { return 0; }

int32_t modem_baudrate() { return settings.modem_baudrate; }
void set_modem_baudrate(const int32_t new_value) { settings.modem_baudrate = new_value; }

int32_t modem_bw() { return 15000; }

uint8_t modem_repeat() { return settings.modem_repeat; }
void set_modem_repeat(const uint32_t new_value) { settings.modem_repeat = new_value; }

bool stealth_mode() { return settings.stealth_mode; }
void set_stealth_mode(const bool v) { settings.stealth_mode = v; }

uint8_t config_cpld() { return settings.config_cpld_val; }
void set_config_cpld(uint8_t i) { settings.config_cpld_val = i; }

bool config_disable_external_tcxo() { return false; }
bool config_sdcard_high_speed_io() { return true; }
bool config_disable_config_mode() { return false; }
bool beep_on_packets() { return false; }

bool config_splash() { return settings.splash_enabled; }
bool config_converter() { return settings.converter_enabled; }
bool config_updown_converter() { return settings.updown_converter; }
int64_t config_converter_freq() { return settings.converter_freq; }
bool show_gui_return_icon() { return settings.gui_return_icon; }
bool hide_clock() { return settings.clock_hidden; }
bool clock_with_date() { return settings.clock_with_date_enabled; }
bool config_login() { return settings.login_enabled; }
bool config_audio_mute() { return settings.audio_muted; }
bool config_speaker_disable() { return settings.speaker_disabled; }
backlight_config_t config_backlight_timer() { return backlight_cfg; }
bool disable_touchscreen() { return settings.touchscreen_disabled; }

void set_gui_return_icon(bool v) { settings.gui_return_icon = v; }
bool load_app_settings() { return settings.load_app_settings_val; }
void set_load_app_settings(bool v) { settings.load_app_settings_val = v; }
bool save_app_settings() { return settings.save_app_settings_val; }
void set_save_app_settings(bool v) { settings.save_app_settings_val = v; }
void set_config_disable_external_tcxo(bool) {}
void set_config_sdcard_high_speed_io(bool, bool) {}
void set_config_disable_config_mode(bool) {}
void set_beep_on_packets(bool) {}

void set_config_splash(bool v) { settings.splash_enabled = v; }
void set_config_converter(bool v) { settings.converter_enabled = v; }
void set_config_updown_converter(bool v) { settings.updown_converter = v; }
void set_config_converter_freq(int64_t v) { settings.converter_freq = v; }
bool config_freq_tx_correction_updown() { return false; }
void set_freq_tx_correction_updown(bool) {}
bool config_freq_rx_correction_updown() { return false; }
void set_freq_rx_correction_updown(bool) {}
uint32_t config_freq_tx_correction() { return 0; }
uint32_t config_freq_rx_correction() { return 0; }
void set_config_freq_tx_correction(uint32_t) {}
void set_config_freq_rx_correction(uint32_t) {}
void set_clock_hidden(bool v) { settings.clock_hidden = v; }
void set_clock_with_date(bool v) { settings.clock_with_date_enabled = v; }
void set_config_login(bool v) { settings.login_enabled = v; }
void set_config_audio_mute(bool v) { settings.audio_muted = v; }
void set_config_speaker_disable(bool v) { settings.speaker_disabled = v; }
void set_config_backlight_timer(const backlight_config_t& new_value) { backlight_cfg = new_value; }
void set_disable_touchscreen(bool v) { settings.touchscreen_disabled = v; }
bool config_lcd_normally_black() { return false; }
void set_lcd_normally_black(bool) {}

uint8_t encoder_dial_sensitivity() { return settings.encoder_sensitivity; }
void set_encoder_dial_sensitivity(uint8_t v) { settings.encoder_sensitivity = v; }
uint8_t encoder_rate_multiplier() { return settings.encoder_rate; }
void set_encoder_rate_multiplier(uint8_t v) { settings.encoder_rate = v; }
bool encoder_dial_direction() { return settings.encoder_dial_dir; }
void set_encoder_dial_direction(bool v) { settings.encoder_dial_dir = v; }

uint32_t config_mode_storage_direct() { return 0; }
void set_config_mode_storage_direct(uint32_t) {}
bool config_disable_config_mode_direct() { return false; }

uint32_t pocsag_last_address() { return settings.pocsag_last_addr; }
void set_pocsag_last_address(uint32_t address) { settings.pocsag_last_addr = address; }

uint32_t pocsag_ignore_address() { return settings.pocsag_ignore_addr; }
void set_pocsag_ignore_address(uint32_t address) { settings.pocsag_ignore_addr = address; }

bool clkout_enabled() { return settings.clkout_on; }
void set_clkout_enabled(bool v) { settings.clkout_on = v; }
void set_clkout_freq(uint16_t freq) { settings.clkout_frequency = freq; }

bool dst_enabled() { return settings.dst_on; }
void set_dst_enabled(bool v) { settings.dst_on = v; }
uint16_t clkout_freq() { return settings.clkout_frequency; }

typedef union {
    uint32_t v;
    struct {
        uint8_t start_which : 4;
        uint8_t start_weekday : 4;
        uint8_t start_month : 4;
        uint8_t end_which : 4;
        uint8_t end_weekday : 4;
        uint8_t end_month : 4;
        uint8_t UNUSED : 7;
        uint8_t dst_enabled : 1;
    } b;
} dst_config_t;

dst_config_t config_dst() { dst_config_t cfg; cfg.v = 0; return cfg; }
void set_config_dst(dst_config_t) {}

bool apply_fake_brightness() { return settings.fake_brightness_on; }
void set_apply_fake_brightness(const bool v) { settings.fake_brightness_on = v; }
uint8_t fake_brightness_level() { return settings.fake_brightness; }
void set_fake_brightness_level(uint8_t v) { settings.fake_brightness = v; }
void toggle_fake_brightness_level() {
    settings.fake_brightness = (settings.fake_brightness % 3) + 1;
}

uint16_t touchscreen_threshold() { return settings.touch_threshold; }
void set_touchscreen_threshold(uint16_t v) { settings.touch_threshold = v; }

ui::Color menu_color() { return settings.menu_color_val; }
void set_menu_color(ui::Color v) { settings.menu_color_val = v; }

// Recon app stubs
uint64_t get_recon_config() { return 0; }
bool recon_autosave_freqs() { return false; }
bool recon_autostart_recon() { return false; }
bool recon_continuous() { return false; }
bool recon_clear_output() { return false; }
bool recon_load_freqs() { return true; }
bool recon_load_repeaters() { return false; }
bool recon_load_ranges() { return true; }
bool recon_update_ranges_when_recon() { return false; }
bool recon_auto_record_locked() { return false; }
bool recon_repeat_recorded() { return false; }
bool recon_repeat_recorded_file_mode() { return false; }
int8_t recon_repeat_nb() { return 0; }
int8_t recon_repeat_gain() { return 0; }
bool recon_repeat_amp() { return false; }
bool recon_load_hamradios() { return false; }
bool recon_match_mode() { return false; }
uint8_t recon_repeat_delay() { return 0; }
void set_recon_autosave_freqs(const bool) {}
void set_recon_autostart_recon(const bool) {}
void set_recon_continuous(const bool) {}
void set_recon_clear_output(const bool) {}
void set_recon_load_freqs(const bool) {}
void set_recon_load_ranges(const bool) {}
void set_recon_update_ranges_when_recon(const bool) {}
void set_recon_auto_record_locked(const bool) {}
void set_recon_repeat_recorded(const bool) {}
void set_recon_repeat_recorded_file_mode(const bool) {}
void set_recon_repeat_nb(const int8_t) {}
void set_recon_repeat_gain(const int8_t) {}
void set_recon_repeat_amp(const bool) {}
void set_recon_load_hamradios(const bool) {}
void set_recon_load_repeaters(const bool) {}
void set_recon_match_mode(const bool) {}
void set_recon_repeat_delay(const uint8_t) {}

// UI Config 2
bool ui_hide_speaker() { return settings.hide_speaker; }
bool ui_hide_mute() { return settings.hide_mute; }
bool ui_hide_converter() { return settings.hide_converter; }
bool ui_hide_stealth() { return settings.hide_stealth; }
bool ui_hide_camera() { return settings.hide_camera; }
bool ui_hide_sleep() { return settings.hide_sleep; }
bool ui_hide_bias_tee() { return settings.hide_bias_tee; }
bool ui_hide_clock() { return settings.hide_clock; }
bool ui_hide_fake_brightness() { return settings.hide_fake_brightness; }
bool ui_hide_numeric_battery() { return settings.hide_numeric_battery; }
bool ui_hide_battery_icon() { return settings.hide_battery_icon; }
bool ui_hide_sd_card() { return settings.hide_sd_card; }
uint8_t ui_theme_id() { return settings.theme_id; }
bool ui_override_batt_calc() { return false; }
bool ui_button_repeat_delay() { return false; }
bool ui_button_repeat_speed() { return false; }
bool ui_button_long_press_delay() { return false; }
bool ui_battery_charge_hint() { return false; }

void set_ui_hide_speaker(bool v) { settings.hide_speaker = v; }
void set_ui_hide_mute(bool v) { settings.hide_mute = v; }
void set_ui_hide_converter(bool v) { settings.hide_converter = v; }
void set_ui_hide_stealth(bool v) { settings.hide_stealth = v; }
void set_ui_hide_camera(bool v) { settings.hide_camera = v; }
void set_ui_hide_sleep(bool v) { settings.hide_sleep = v; }
void set_ui_hide_bias_tee(bool v) { settings.hide_bias_tee = v; }
void set_ui_hide_clock(bool v) { settings.hide_clock = v; }
void set_ui_hide_fake_brightness(bool v) { settings.hide_fake_brightness = v; }
void set_ui_hide_numeric_battery(bool v) { settings.hide_numeric_battery = v; }
void set_ui_hide_battery_icon(bool v) { settings.hide_battery_icon = v; }
void set_ui_hide_sd_card(bool v) { settings.hide_sd_card = v; }
void set_ui_theme_id(uint8_t v) { settings.theme_id = v; }
void set_ui_override_batt_calc(bool) {}
void set_ui_button_repeat_delay(bool) {}
void set_ui_button_repeat_speed(bool) {}
void set_ui_button_long_press_delay(bool) {}
void set_ui_battery_charge_hint(bool) {}

// SD card settings persistence
bool should_use_sdcard_for_pmem() { return false; }
int save_persistent_settings_to_file() { return 0; }
int load_persistent_settings_from_file() { return 0; }

uint32_t get_data_structure_version() { return 0x00010000; }
uint32_t pmem_data_word(uint32_t) { return 0; }
uint32_t pmem_stored_checksum() { return 0; }
uint32_t pmem_calculated_checksum() { return 0; }

size_t data_size() { return sizeof(settings); }

} // namespace persistent_memory
} // namespace portapack

#endif // PORTAPACK_PC_EMULATOR
