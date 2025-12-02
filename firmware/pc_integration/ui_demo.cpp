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

#include "ui_demo.hpp"
#include "portapack_shim.hpp"
#include "display_shim.hpp"
#include "event_shim.hpp"
#include "receiver_model_shim.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace ui_demo {

// ============================================================================
// Painter Implementation
// ============================================================================

void Painter::fill_rect(const Rect& rect, const Color& color) {
    shim::get_display().fill_rectangle(rect.x, rect.y, rect.w, rect.h, color.to_rgb565());
}

void Painter::draw_rect(const Rect& rect, const Color& color) {
    auto& disp = shim::get_display();
    uint16_t c = color.to_rgb565();
    // Top
    disp.fill_rectangle(rect.x, rect.y, rect.w, 1, c);
    // Bottom
    disp.fill_rectangle(rect.x, rect.y + rect.h - 1, rect.w, 1, c);
    // Left
    disp.fill_rectangle(rect.x, rect.y, 1, rect.h, c);
    // Right
    disp.fill_rectangle(rect.x + rect.w - 1, rect.y, 1, rect.h, c);
}

void Painter::draw_hline(int x, int y, int w, const Color& color) {
    shim::get_display().fill_rectangle(x, y, w, 1, color.to_rgb565());
}

void Painter::draw_vline(int x, int y, int h, const Color& color) {
    shim::get_display().fill_rectangle(x, y, 1, h, color.to_rgb565());
}

// Simple 8x8 font character drawing (built-in basic font)
static const uint8_t font_8x8[][8] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},
    // " (34)
    {0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # through / omitted for brevity, filled with zeros
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    // 0 (48)
    {0x7C, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00},
    // 1 (49)
    {0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00},
    // 2 (50)
    {0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00},
    // 3 (51)
    {0x78, 0xCC, 0x0C, 0x38, 0x0C, 0xCC, 0x78, 0x00},
    // 4 (52)
    {0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00},
    // 5 (53)
    {0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00},
    // 6 (54)
    {0x38, 0x60, 0xC0, 0xF8, 0xCC, 0xCC, 0x78, 0x00},
    // 7 (55)
    {0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
    // 8 (56)
    {0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00},
    // 9 (57)
    {0x78, 0xCC, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00},
    // : through @ omitted
    {0},{0},{0},{0},{0},{0},{0},
    // A (65)
    {0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00},
    // B (66)
    {0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00},
    // C (67)
    {0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00},
    // D (68)
    {0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00},
    // E (69)
    {0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00},
    // F (70)
    {0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00},
    // G (71)
    {0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00},
    // H (72)
    {0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00},
    // I (73)
    {0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},
    // J (74)
    {0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00},
    // K (75)
    {0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00},
    // L (76)
    {0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00},
    // M (77)
    {0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00},
    // N (78)
    {0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00},
    // O (79)
    {0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00},
    // P (80)
    {0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00},
    // Q (81)
    {0x78, 0xCC, 0xCC, 0xCC, 0xDC, 0x78, 0x1C, 0x00},
    // R (82)
    {0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00},
    // S (83)
    {0x78, 0xCC, 0xE0, 0x70, 0x1C, 0xCC, 0x78, 0x00},
    // T (84)
    {0xFC, 0xB4, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},
    // U (85)
    {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x00},
    // V (86)
    {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00},
    // W (87)
    {0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00},
    // X (88)
    {0xC6, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x00},
    // Y (89)
    {0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00},
    // Z (90)
    {0xFE, 0xC6, 0x8C, 0x18, 0x32, 0x66, 0xFE, 0x00},
    // [ through ` omitted
    {0},{0},{0},{0},{0},{0},
    // a (97)
    {0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00},
    // b (98)
    {0xE0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xDC, 0x00},
    // c (99)
    {0x00, 0x00, 0x78, 0xCC, 0xC0, 0xCC, 0x78, 0x00},
    // d (100)
    {0x1C, 0x0C, 0x0C, 0x7C, 0xCC, 0xCC, 0x76, 0x00},
    // e (101)
    {0x00, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00},
    // f (102)
    {0x38, 0x6C, 0x60, 0xF0, 0x60, 0x60, 0xF0, 0x00},
    // g (103)
    {0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8},
    // h (104)
    {0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00},
    // i (105)
    {0x30, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00},
    // j (106)
    {0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78},
    // k (107)
    {0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00},
    // l (108)
    {0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},
    // m (109)
    {0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xC6, 0x00},
    // n (110)
    {0x00, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00},
    // o (111)
    {0x00, 0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0x00},
    // p (112)
    {0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0},
    // q (113)
    {0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E},
    // r (114)
    {0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0xF0, 0x00},
    // s (115)
    {0x00, 0x00, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x00},
    // t (116)
    {0x10, 0x30, 0x7C, 0x30, 0x30, 0x34, 0x18, 0x00},
    // u (117)
    {0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00},
    // v (118)
    {0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00},
    // w (119)
    {0x00, 0x00, 0xC6, 0xD6, 0xFE, 0xFE, 0x6C, 0x00},
    // x (120)
    {0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00},
    // y (121)
    {0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8},
    // z (122)
    {0x00, 0x00, 0xFC, 0x98, 0x30, 0x64, 0xFC, 0x00},
};

static int font_index(char c) {
    if (c >= 32 && c <= 122) {
        return c - 32;
    }
    return 0; // Space for unknown
}

void Painter::draw_char(int x, int y, char c, const Color& fg, const Color& bg) {
    int idx = font_index(c);
    if (idx < 0 || idx >= static_cast<int>(sizeof(font_8x8) / sizeof(font_8x8[0]))) {
        idx = 0;
    }

    auto& disp = shim::get_display();
    uint16_t fg_c = fg.to_rgb565();
    uint16_t bg_c = bg.to_rgb565();

    for (int row = 0; row < 8; row++) {
        uint8_t bits = font_8x8[idx][row];
        for (int col = 0; col < 8; col++) {
            bool set = (bits >> (7 - col)) & 1;
            disp.draw_pixel(x + col, y + row, set ? fg_c : bg_c);
        }
    }
}

void Painter::draw_text(int x, int y, const std::string& text, const Color& fg, const Color& bg) {
    int cx = x;
    for (char c : text) {
        draw_char(cx, y, c, fg, bg);
        cx += 8;
    }
}

// ============================================================================
// Text Widget
// ============================================================================

void Text::paint(Painter& painter) {
    painter.fill_rect(bounds_, bg_);
    painter.draw_text(bounds_.x + 2, bounds_.y + 2, text_, fg_, bg_);
    clear_dirty();
}

// ============================================================================
// Button Widget
// ============================================================================

void Button::paint(Painter& painter) {
    Color bg = has_focus() ? colors::blue : colors::dark_grey;
    Color border = has_focus() ? colors::cyan : colors::grey;

    painter.fill_rect(bounds_, bg);
    painter.draw_rect(bounds_, border);

    // Center text
    int text_x = bounds_.x + (bounds_.w - static_cast<int>(label_.size()) * 8) / 2;
    int text_y = bounds_.y + (bounds_.h - 8) / 2;
    painter.draw_text(text_x, text_y, label_, colors::white, bg);

    clear_dirty();
}

bool Button::on_key(KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (on_select) {
            on_select();
        }
        return true;
    }
    return false;
}

bool Button::on_touch(const TouchEvent& event) {
    if (event.type == TouchType::Start && bounds_.contains(event.point.x, event.point.y)) {
        if (on_select) {
            on_select();
        }
        return true;
    }
    return false;
}

// ============================================================================
// MenuView Widget
// ============================================================================

MenuView::MenuView(const Rect& rect) : Widget(rect) {}

void MenuView::add_item(const MenuItem& item) {
    items_.push_back(item);
    set_dirty();
}

void MenuView::clear_items() {
    items_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
    set_dirty();
}

void MenuView::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::black);

    int y = bounds_.y;
    int visible_start = scroll_offset_;
    int visible_end = std::min(static_cast<int>(items_.size()), scroll_offset_ + VISIBLE_ITEMS);

    for (int i = visible_start; i < visible_end; i++) {
        const auto& item = items_[i];
        bool selected = (static_cast<size_t>(i) == selected_index_);

        Rect item_rect{bounds_.x, static_cast<int16_t>(y), bounds_.w, ITEM_HEIGHT};

        if (selected) {
            painter.fill_rect(item_rect, item.color);
            painter.draw_text(item_rect.x + 4, item_rect.y + 8, item.label, colors::white, item.color);
        } else {
            painter.fill_rect(item_rect, colors::dark_grey);
            painter.draw_text(item_rect.x + 4, item_rect.y + 8, item.label, item.color, colors::dark_grey);
        }

        // Draw separator
        painter.draw_hline(bounds_.x, y + ITEM_HEIGHT - 1, bounds_.w, colors::grey);

        y += ITEM_HEIGHT;
    }

    clear_dirty();
}

bool MenuView::on_key(KeyEvent key) {
    switch (key) {
        case KeyEvent::Up:
            if (selected_index_ > 0) {
                selected_index_--;
                ensure_visible();
                set_dirty();
            }
            return true;

        case KeyEvent::Down:
            if (selected_index_ < items_.size() - 1) {
                selected_index_++;
                ensure_visible();
                set_dirty();
            }
            return true;

        case KeyEvent::Select:
            select_item();
            return true;

        default:
            return false;
    }
}

bool MenuView::on_touch(const TouchEvent& event) {
    if (event.type != TouchType::Start) return false;
    if (!bounds_.contains(event.point.x, event.point.y)) return false;

    int rel_y = event.point.y - bounds_.y;
    int item_idx = scroll_offset_ + (rel_y / ITEM_HEIGHT);

    if (item_idx >= 0 && static_cast<size_t>(item_idx) < items_.size()) {
        selected_index_ = item_idx;
        set_dirty();
        select_item();
        return true;
    }

    return false;
}

bool MenuView::on_encoder(int32_t delta) {
    if (delta > 0 && selected_index_ < items_.size() - 1) {
        selected_index_++;
        ensure_visible();
        set_dirty();
    } else if (delta < 0 && selected_index_ > 0) {
        selected_index_--;
        ensure_visible();
        set_dirty();
    }
    return true;
}

void MenuView::select_item() {
    if (selected_index_ < items_.size() && items_[selected_index_].on_select) {
        items_[selected_index_].on_select();
    }
}

void MenuView::ensure_visible() {
    if (static_cast<int>(selected_index_) < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (static_cast<int>(selected_index_) >= scroll_offset_ + VISIBLE_ITEMS) {
        scroll_offset_ = selected_index_ - VISIBLE_ITEMS + 1;
    }
}

// ============================================================================
// View
// ============================================================================

void View::add_child(std::shared_ptr<Widget> child) {
    children_.push_back(child);
    if (children_.size() == 1) {
        child->focus();
    }
}

void View::remove_all_children() {
    children_.clear();
    focused_child_ = 0;
}

void View::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::black);
    for (auto& child : children_) {
        child->paint(painter);
    }
    clear_dirty();
}

bool View::on_key(KeyEvent key) {
    // Navigation between children
    if (key == KeyEvent::Down || key == KeyEvent::Right) {
        if (children_.size() > 1) {
            if (!children_.empty() && focused_child_ < children_.size()) {
                if (children_[focused_child_]->on_key(key)) {
                    return true;
                }
            }
            focus_next();
            set_dirty();
            return true;
        }
    } else if (key == KeyEvent::Up || key == KeyEvent::Left) {
        if (children_.size() > 1) {
            if (!children_.empty() && focused_child_ < children_.size()) {
                if (children_[focused_child_]->on_key(key)) {
                    return true;
                }
            }
            focus_prev();
            set_dirty();
            return true;
        }
    }

    // Pass to focused child
    if (!children_.empty() && focused_child_ < children_.size()) {
        return children_[focused_child_]->on_key(key);
    }

    return false;
}

bool View::on_touch(const TouchEvent& event) {
    for (auto& child : children_) {
        if (child->bounds().contains(event.point.x, event.point.y)) {
            return child->on_touch(event);
        }
    }
    return false;
}

bool View::on_encoder(int32_t delta) {
    if (!children_.empty() && focused_child_ < children_.size()) {
        return children_[focused_child_]->on_encoder(delta);
    }
    return false;
}

void View::focus_next() {
    if (children_.empty()) return;

    if (focused_child_ < children_.size()) {
        children_[focused_child_]->blur();
    }

    focused_child_ = (focused_child_ + 1) % children_.size();
    children_[focused_child_]->focus();
}

void View::focus_prev() {
    if (children_.empty()) return;

    if (focused_child_ < children_.size()) {
        children_[focused_child_]->blur();
    }

    focused_child_ = (focused_child_ == 0) ? children_.size() - 1 : focused_child_ - 1;
    children_[focused_child_]->focus();
}

// ============================================================================
// NavigationView
// ============================================================================

NavigationView::NavigationView() {}

void NavigationView::push(std::shared_ptr<View> view) {
    view_stack_.push_back(view);
    std::cout << "NavigationView: Pushed " << view->title() << ", stack size: " << view_stack_.size() << std::endl;
}

void NavigationView::pop() {
    if (view_stack_.size() > 1) {
        std::cout << "NavigationView: Popping " << view_stack_.back()->title() << std::endl;
        view_stack_.pop_back();
    }
}

void NavigationView::home() {
    while (view_stack_.size() > 1) {
        view_stack_.pop_back();
    }
    std::cout << "NavigationView: Returned to home" << std::endl;
}

View* NavigationView::current_view() {
    return view_stack_.empty() ? nullptr : view_stack_.back().get();
}

void NavigationView::paint(Painter& painter) {
    if (auto* view = current_view()) {
        view->paint(painter);
    }
}

bool NavigationView::on_key(KeyEvent key) {
    if (auto* view = current_view()) {
        return view->on_key(key);
    }
    return false;
}

bool NavigationView::on_touch(const TouchEvent& event) {
    if (auto* view = current_view()) {
        return view->on_touch(event);
    }
    return false;
}

bool NavigationView::on_encoder(int32_t delta) {
    if (auto* view = current_view()) {
        return view->on_encoder(delta);
    }
    return false;
}

// ============================================================================
// Menu Views
// ============================================================================

HomeView::HomeView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{0, 16, 240, 16}, "PortaPack Mayhem");
    title_text_->set_colors(colors::yellow, colors::dark_blue);
    add_child(title_text_);

    menu_ = std::make_shared<MenuView>(Rect{0, 36, 240, 280});
    menu_->add_item({"Receive", colors::green, [this]() {
        nav_.push(std::make_shared<ReceiveMenuView>(nav_));
    }});
    menu_->add_item({"Transmit", colors::red, [this]() {
        nav_.push(std::make_shared<TransmitMenuView>(nav_));
    }});
    menu_->add_item({"Utilities", colors::cyan, [this]() {
        nav_.push(std::make_shared<UtilitiesMenuView>(nav_));
    }});
    menu_->add_item({"Settings", colors::yellow, [this]() {
        nav_.push(std::make_shared<SettingsMenuView>(nav_));
    }});
    menu_->add_item({"About", colors::magenta, [this]() {
        nav_.push(std::make_shared<AboutView>(nav_));
    }});
    add_child(menu_);
}

ReceiveMenuView::ReceiveMenuView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{0, 16, 240, 16}, "Receive Apps");
    title_text_->set_colors(colors::green, colors::dark_blue);
    add_child(title_text_);

    menu_ = std::make_shared<MenuView>(Rect{0, 36, 240, 280});
    menu_->add_item({"Audio (NFM/AM/WFM)", colors::green, [this]() {
        nav_.push(std::make_shared<AudioReceiverView>(nav_));
    }});
    menu_->add_item({"Spectrum Analyzer", colors::cyan, [this]() {
        nav_.push(std::make_shared<SpectrumAnalyzerView>(nav_));
    }});
    menu_->add_item({"ADS-B", colors::green, []() {
        std::cout << "Selected: ADS-B Receiver" << std::endl;
    }});
    menu_->add_item({"POCSAG", colors::green, []() {
        std::cout << "Selected: POCSAG Receiver" << std::endl;
    }});
    menu_->add_item({"AIS", colors::green, []() {
        std::cout << "Selected: AIS Receiver" << std::endl;
    }});
    menu_->add_item({"APRS", colors::green, []() {
        std::cout << "Selected: APRS Receiver" << std::endl;
    }});
    menu_->add_item({"Weather Station", colors::green, []() {
        std::cout << "Selected: Weather Station" << std::endl;
    }});
    menu_->add_item({"BLE RX", colors::green, []() {
        std::cout << "Selected: BLE Receiver" << std::endl;
    }});
    add_child(menu_);
}

TransmitMenuView::TransmitMenuView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{0, 16, 240, 16}, "Transmit Apps");
    title_text_->set_colors(colors::red, colors::dark_blue);
    add_child(title_text_);

    menu_ = std::make_shared<MenuView>(Rect{0, 36, 240, 280});
    menu_->add_item({"Microphone TX", colors::red, []() {
        std::cout << "Selected: Microphone TX" << std::endl;
    }});
    menu_->add_item({"POCSAG TX", colors::red, []() {
        std::cout << "Selected: POCSAG TX" << std::endl;
    }});
    menu_->add_item({"RDS TX", colors::red, []() {
        std::cout << "Selected: RDS TX" << std::endl;
    }});
    menu_->add_item({"Signal Generator", colors::red, []() {
        std::cout << "Selected: Signal Generator" << std::endl;
    }});
    menu_->add_item({"TouchTunes", colors::red, []() {
        std::cout << "Selected: TouchTunes" << std::endl;
    }});
    add_child(menu_);
}

UtilitiesMenuView::UtilitiesMenuView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{0, 16, 240, 16}, "Utilities");
    title_text_->set_colors(colors::cyan, colors::dark_blue);
    add_child(title_text_);

    menu_ = std::make_shared<MenuView>(Rect{0, 36, 240, 280});
    menu_->add_item({"File Manager", colors::cyan, []() {
        std::cout << "Selected: File Manager" << std::endl;
    }});
    menu_->add_item({"Frequency Manager", colors::cyan, []() {
        std::cout << "Selected: Frequency Manager" << std::endl;
    }});
    menu_->add_item({"Signal Database", colors::cyan, []() {
        std::cout << "Selected: Signal Database" << std::endl;
    }});
    menu_->add_item({"Text Editor", colors::cyan, []() {
        std::cout << "Selected: Text Editor" << std::endl;
    }});
    menu_->add_item({"Flash Utility", colors::cyan, []() {
        std::cout << "Selected: Flash Utility" << std::endl;
    }});
    add_child(menu_);
}

SettingsMenuView::SettingsMenuView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{0, 16, 240, 16}, "Settings");
    title_text_->set_colors(colors::yellow, colors::dark_blue);
    add_child(title_text_);

    menu_ = std::make_shared<MenuView>(Rect{0, 36, 240, 280});
    menu_->add_item({"Display", colors::yellow, []() {
        std::cout << "Selected: Display Settings" << std::endl;
    }});
    menu_->add_item({"Audio", colors::yellow, []() {
        std::cout << "Selected: Audio Settings" << std::endl;
    }});
    menu_->add_item({"Radio", colors::yellow, []() {
        std::cout << "Selected: Radio Settings" << std::endl;
    }});
    menu_->add_item({"Date/Time", colors::yellow, []() {
        std::cout << "Selected: Date/Time Settings" << std::endl;
    }});
    menu_->add_item({"Touch Calibration", colors::yellow, []() {
        std::cout << "Selected: Touch Calibration" << std::endl;
    }});
    add_child(menu_);
}

AboutView::AboutView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    title_text_ = std::make_shared<Text>(Rect{10, 40, 220, 16}, "PortaPack Mayhem");
    title_text_->set_colors(colors::yellow, colors::black);
    add_child(title_text_);

    version_text_ = std::make_shared<Text>(Rect{10, 60, 220, 16}, "PC Emulator v1.0.0");
    version_text_->set_colors(colors::white, colors::black);
    add_child(version_text_);

    info_text_ = std::make_shared<Text>(Rect{10, 100, 220, 16}, "Press Back to return");
    info_text_->set_colors(colors::grey, colors::black);
    add_child(info_text_);
}

// ============================================================================
// BigFrequency Widget
// ============================================================================

// Big digit font (16x24 pixels per digit)
static const uint32_t big_digits[10][24] = {
    // 0
    {0x07E0, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007,
     0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0x700E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000},
    // 1
    {0x0380, 0x0780, 0x0F80, 0x1F80, 0x3B80, 0x7380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,
     0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x7FFE, 0x7FFE, 0x7FFE, 0x0000, 0x0000},
    // 2
    {0x0FF0, 0x3FFC, 0x783E, 0x600E, 0x0006, 0x0006, 0x000E, 0x001C, 0x0038, 0x0070, 0x00E0, 0x01C0,
     0x0380, 0x0700, 0x0E00, 0x1C00, 0x3800, 0x7000, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x0000, 0x0000},
    // 3
    {0x0FF0, 0x3FFC, 0x781E, 0x600E, 0x0006, 0x0006, 0x000E, 0x003C, 0x0FF0, 0x0FF0, 0x003C, 0x000E,
     0x0006, 0x0006, 0x0006, 0x0006, 0x600E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000, 0x0000},
    // 4
    {0x0030, 0x0070, 0x00F0, 0x01F0, 0x03B0, 0x0730, 0x0E30, 0x1C30, 0x3830, 0x7030, 0xE030, 0xFFFF,
     0xFFFF, 0xFFFF, 0x0030, 0x0030, 0x0030, 0x0030, 0x0030, 0x0030, 0x0030, 0x0000, 0x0000, 0x0000},
    // 5
    {0x7FFE, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000, 0x7FE0, 0x7FF8, 0x7FFC, 0x001E, 0x0006, 0x0006,
     0x0006, 0x0006, 0x0006, 0x0006, 0x600E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000, 0x0000},
    // 6
    {0x07E0, 0x1FF8, 0x3FFC, 0x781E, 0x7000, 0xE000, 0xE000, 0xE7E0, 0xEFF8, 0xFFFC, 0xF81E, 0xF00E,
     0xE007, 0xE007, 0xE007, 0x700E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000, 0x0000, 0x0000},
    // 7
    {0x7FFE, 0x7FFE, 0x7FFE, 0x000E, 0x001C, 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0380, 0x0700, 0x0700,
     0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0000, 0x0000, 0x0000},
    // 8
    {0x07E0, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x381C, 0x1FF8, 0x07E0, 0x1FF8, 0x381C,
     0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000, 0x0000, 0x0000},
    // 9
    {0x07E0, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0xE007, 0xE007, 0xE007, 0x700F, 0x781F, 0x3FFF, 0x1FF7,
     0x07E7, 0x0007, 0x0007, 0x000E, 0x781E, 0x3FFC, 0x1FF8, 0x07E0, 0x0000, 0x0000, 0x0000, 0x0000},
};

BigFrequency::BigFrequency(const Rect& rect, int64_t initial_freq)
    : Widget(rect), frequency_(initial_freq) {}

void BigFrequency::set_frequency(int64_t freq_hz) {
    if (freq_hz != frequency_) {
        frequency_ = freq_hz;
        set_dirty();
        if (on_change) {
            on_change(frequency_);
        }
    }
}

void BigFrequency::draw_big_digit(Painter& painter, int x, int y, char digit, const Color& fg, const Color& bg) {
    if (digit < '0' || digit > '9') return;

    int idx = digit - '0';
    auto& disp = shim::get_display();
    uint16_t fg_c = fg.to_rgb565();
    uint16_t bg_c = bg.to_rgb565();

    for (int row = 0; row < 20; row++) {
        uint32_t bits = big_digits[idx][row];
        for (int col = 0; col < 16; col++) {
            bool set = (bits >> (15 - col)) & 1;
            disp.draw_pixel(x + col, y + row, set ? fg_c : bg_c);
        }
    }
}

void BigFrequency::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::black);

    // Format frequency as XXX.XXX.XXX
    char buf[16];
    int mhz = static_cast<int>(frequency_ / 1000000);
    int khz = static_cast<int>((frequency_ % 1000000) / 1000);
    int hz = static_cast<int>(frequency_ % 1000);
    snprintf(buf, sizeof(buf), "%3d.%03d.%03d", mhz, khz, hz);

    // Draw digits with larger spacing
    int x = bounds_.x + 4;
    int y = bounds_.y + 4;
    int digit_width = 14;
    int dot_width = 6;

    for (int i = 0; buf[i]; i++) {
        Color fg = colors::green;
        Color bg = colors::black;

        // Highlight selected digit
        if (has_focus() && i == selected_digit_) {
            fg = colors::yellow;
            bg = Color{32, 32, 0};
        }

        if (buf[i] == '.') {
            painter.fill_rect(Rect{static_cast<int16_t>(x), static_cast<int16_t>(y + 16), 4, 4}, fg);
            x += dot_width;
        } else {
            draw_big_digit(painter, x, y, buf[i], fg, bg);
            x += digit_width;
        }
    }

    // Draw step indicator
    std::string step_str;
    if (step_ >= 1000000) step_str = std::to_string(step_ / 1000000) + " MHz";
    else if (step_ >= 1000) step_str = std::to_string(step_ / 1000) + " kHz";
    else step_str = std::to_string(step_) + " Hz";

    painter.draw_text(bounds_.x + 4, bounds_.y + bounds_.h - 10, "Step: " + step_str, colors::grey, colors::black);

    clear_dirty();
}

bool BigFrequency::on_encoder(int32_t delta) {
    set_frequency(frequency_ + delta * step_);
    return true;
}

bool BigFrequency::on_key(KeyEvent key) {
    switch (key) {
        case KeyEvent::Left:
            if (selected_digit_ > 0) {
                selected_digit_--;
                if (selected_digit_ == 3 || selected_digit_ == 7) selected_digit_--;  // Skip dots
                set_dirty();
            }
            return true;
        case KeyEvent::Right:
            if (selected_digit_ < 10) {
                selected_digit_++;
                if (selected_digit_ == 3 || selected_digit_ == 7) selected_digit_++;
                set_dirty();
            }
            return true;
        case KeyEvent::Up:
            set_frequency(frequency_ + step_);
            return true;
        case KeyEvent::Down:
            set_frequency(frequency_ - step_);
            return true;
        case KeyEvent::Select:
            // Cycle through step sizes
            if (step_ >= 10000000) step_ = 1;
            else step_ *= 10;
            set_dirty();
            return true;
        default:
            return false;
    }
}

// ============================================================================
// RSSIMeter Widget
// ============================================================================

RSSIMeter::RSSIMeter(const Rect& rect) : Widget(rect) {}

void RSSIMeter::set_value(int db) {
    current_db_ = std::max(MIN_DB, std::min(MAX_DB, db));
    if (current_db_ > peak_db_) {
        peak_db_ = current_db_;
    }
    set_dirty();
}

Color RSSIMeter::db_to_color(int db) const {
    if (db >= -30) return colors::red;
    if (db >= -50) return colors::yellow;
    if (db >= -70) return colors::green;
    return Color{0, 128, 0};  // Dark green
}

void RSSIMeter::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::black);

    // Draw border
    painter.draw_rect(bounds_, colors::grey);

    // Calculate bar width
    int range = MAX_DB - MIN_DB;
    int bar_width = bounds_.w - 4;
    int current_width = ((current_db_ - MIN_DB) * bar_width) / range;
    int peak_x = ((peak_db_ - MIN_DB) * bar_width) / range;

    // Draw gradient bar
    for (int x = 0; x < current_width; x++) {
        int db_at_x = MIN_DB + (x * range) / bar_width;
        Color c = db_to_color(db_at_x);
        painter.draw_vline(bounds_.x + 2 + x, bounds_.y + 2, bounds_.h - 4, c);
    }

    // Draw peak marker
    if (peak_x > 0 && peak_x < bar_width) {
        painter.draw_vline(bounds_.x + 2 + peak_x, bounds_.y + 2, bounds_.h - 4, colors::white);
    }

    // Draw dB labels
    painter.draw_text(bounds_.x + 2, bounds_.y + bounds_.h + 2,
                      std::to_string(current_db_) + " dB", colors::green, colors::black);

    clear_dirty();
}

// ============================================================================
// SpectrumWidget
// ============================================================================

SpectrumWidget::SpectrumWidget(const Rect& rect) : Widget(rect) {
    spectrum_data_.resize(rect.w, 0);
}

void SpectrumWidget::set_data(const std::vector<uint8_t>& data) {
    spectrum_data_ = data;
    if (spectrum_data_.size() != static_cast<size_t>(bounds_.w)) {
        spectrum_data_.resize(bounds_.w, 0);
    }
    set_dirty();
}

Color SpectrumWidget::amplitude_to_color(uint8_t amplitude) const {
    if (amplitude > 200) return colors::red;
    if (amplitude > 150) return colors::yellow;
    if (amplitude > 100) return colors::green;
    if (amplitude > 50) return colors::cyan;
    return colors::blue;
}

void SpectrumWidget::generate_demo_data() {
    // Generate realistic-looking spectrum with noise floor and some signals
    static int phase = 0;
    phase++;

    for (size_t i = 0; i < spectrum_data_.size(); i++) {
        // Base noise floor around 30-50
        int noise = 30 + (rand() % 20);

        // Add some simulated signals
        int center = bounds_.w / 2;
        int dist = std::abs(static_cast<int>(i) - center);

        // Main signal at center
        if (dist < 10) {
            noise += 150 - dist * 10;
        }

        // Side signals
        if (std::abs(static_cast<int>(i) - center - 50) < 5) {
            noise += 80;
        }
        if (std::abs(static_cast<int>(i) - center + 40) < 3) {
            noise += 100;
        }

        // Animate slightly
        noise += (rand() % 10) - 5;
        noise = std::max(0, std::min(255, noise));

        spectrum_data_[i] = static_cast<uint8_t>(noise);
    }
    set_dirty();
}

void SpectrumWidget::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::black);

    // Draw grid lines
    for (int y = bounds_.h / 4; y < bounds_.h; y += bounds_.h / 4) {
        for (int x = 0; x < bounds_.w; x += 4) {
            shim::get_display().draw_pixel(bounds_.x + x, bounds_.y + y, colors::dark_grey.to_rgb565());
        }
    }

    // Draw spectrum
    for (size_t i = 0; i < spectrum_data_.size() && static_cast<int>(i) < bounds_.w; i++) {
        int height = (spectrum_data_[i] * bounds_.h) / 256;
        Color c = amplitude_to_color(spectrum_data_[i]);

        for (int y = 0; y < height; y++) {
            shim::get_display().draw_pixel(
                bounds_.x + i,
                bounds_.y + bounds_.h - 1 - y,
                c.to_rgb565()
            );
        }
    }

    // Draw center frequency marker
    painter.draw_vline(bounds_.x + bounds_.w / 2, bounds_.y, bounds_.h, Color{64, 64, 64});

    clear_dirty();
}

// ============================================================================
// WaterfallWidget
// ============================================================================

WaterfallWidget::WaterfallWidget(const Rect& rect) : Widget(rect) {}

Color WaterfallWidget::amplitude_to_color(uint8_t amplitude) const {
    // Blue -> Cyan -> Green -> Yellow -> Red
    if (amplitude < 64) {
        return Color{0, 0, static_cast<uint8_t>(amplitude * 4)};
    } else if (amplitude < 128) {
        return Color{0, static_cast<uint8_t>((amplitude - 64) * 4), 255};
    } else if (amplitude < 192) {
        return Color{0, 255, static_cast<uint8_t>(255 - (amplitude - 128) * 4)};
    } else {
        return Color{static_cast<uint8_t>((amplitude - 192) * 4), 255, 0};
    }
}

void WaterfallWidget::add_line(const std::vector<uint8_t>& data) {
    waterfall_data_.insert(waterfall_data_.begin(), data);
    if (waterfall_data_.size() > MAX_LINES) {
        waterfall_data_.pop_back();
    }
    set_dirty();
}

void WaterfallWidget::paint(Painter& painter) {
    auto& disp = shim::get_display();

    for (size_t row = 0; row < waterfall_data_.size() && static_cast<int>(row) < bounds_.h; row++) {
        const auto& line = waterfall_data_[row];
        for (size_t col = 0; col < line.size() && static_cast<int>(col) < bounds_.w; col++) {
            Color c = amplitude_to_color(line[col]);
            disp.draw_pixel(bounds_.x + col, bounds_.y + row, c.to_rgb565());
        }
    }

    clear_dirty();
}

// ============================================================================
// OptionsField Widget
// ============================================================================

OptionsField::OptionsField(const Rect& rect, const std::vector<option_t>& options)
    : Widget(rect), options_(options) {}

void OptionsField::set_selected_index(size_t index) {
    if (index < options_.size() && index != selected_index_) {
        selected_index_ = index;
        set_dirty();
        if (on_change) {
            on_change(selected_index_, selected_value());
        }
    }
}

int32_t OptionsField::selected_value() const {
    return selected_index_ < options_.size() ? options_[selected_index_].second : 0;
}

void OptionsField::paint(Painter& painter) {
    Color bg = has_focus() ? colors::blue : colors::dark_grey;
    Color fg = colors::white;

    painter.fill_rect(bounds_, bg);
    painter.draw_rect(bounds_, has_focus() ? colors::cyan : colors::grey);

    if (selected_index_ < options_.size()) {
        // Draw arrows
        painter.draw_text(bounds_.x + 2, bounds_.y + 4, "<", fg, bg);
        painter.draw_text(bounds_.x + bounds_.w - 10, bounds_.y + 4, ">", fg, bg);

        // Draw option text centered
        const auto& text = options_[selected_index_].first;
        int text_x = bounds_.x + (bounds_.w - static_cast<int>(text.size()) * 8) / 2;
        painter.draw_text(text_x, bounds_.y + 4, text, fg, bg);
    }

    clear_dirty();
}

bool OptionsField::on_key(KeyEvent key) {
    switch (key) {
        case KeyEvent::Left:
            if (selected_index_ > 0) {
                set_selected_index(selected_index_ - 1);
            }
            return true;
        case KeyEvent::Right:
            if (selected_index_ < options_.size() - 1) {
                set_selected_index(selected_index_ + 1);
            }
            return true;
        default:
            return false;
    }
}

bool OptionsField::on_encoder(int32_t delta) {
    if (delta > 0 && selected_index_ < options_.size() - 1) {
        set_selected_index(selected_index_ + 1);
    } else if (delta < 0 && selected_index_ > 0) {
        set_selected_index(selected_index_ - 1);
    }
    return true;
}

// ============================================================================
// AudioReceiverView
// ============================================================================

AudioReceiverView::AudioReceiverView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    // Frequency display
    frequency_ = std::make_shared<BigFrequency>(Rect{0, 16, 240, 36}, 100000000);
    frequency_->on_change = [this](int64_t freq) {
        shim::get_receiver_model().set_target_frequency(freq);
        spectrum_->set_center_frequency(freq);
    };
    add_child(frequency_);

    // RSSI meter
    rssi_ = std::make_shared<RSSIMeter>(Rect{0, 56, 240, 16});
    add_child(rssi_);

    // Spectrum display
    spectrum_ = std::make_shared<SpectrumWidget>(Rect{0, 76, 240, 60});
    add_child(spectrum_);

    // Waterfall display
    waterfall_ = std::make_shared<WaterfallWidget>(Rect{0, 140, 240, 80});
    add_child(waterfall_);

    // Mode label and selector
    mode_label_ = std::make_shared<Text>(Rect{4, 226, 40, 16}, "Mode:");
    mode_label_->set_colors(colors::grey, colors::black);
    add_child(mode_label_);

    modulation_ = std::make_shared<OptionsField>(
        Rect{48, 224, 80, 20},
        std::vector<OptionsField::option_t>{
            {"NFM", 0},
            {"WFM", 1},
            {"AM", 2},
            {"USB", 3},
            {"LSB", 4},
        }
    );
    add_child(modulation_);

    // Bandwidth label and selector
    bw_label_ = std::make_shared<Text>(Rect{136, 226, 24, 16}, "BW:");
    bw_label_->set_colors(colors::grey, colors::black);
    add_child(bw_label_);

    bandwidth_ = std::make_shared<OptionsField>(
        Rect{160, 224, 76, 20},
        std::vector<OptionsField::option_t>{
            {"8.5k", 8500},
            {"11k", 11000},
            {"16k", 16000},
            {"200k", 200000},
        }
    );
    add_child(bandwidth_);
}

void AudioReceiverView::update_demo_data() {
    // Update RSSI with random variation
    int rssi_val = -60 + (rand() % 20) - 10;
    rssi_->set_value(rssi_val);

    // Generate demo spectrum
    spectrum_->generate_demo_data();

    // Copy spectrum to waterfall
    std::vector<uint8_t> line(240);
    for (int i = 0; i < 240; i++) {
        line[i] = 30 + (rand() % 50);
        // Add signal at center
        int dist = std::abs(i - 120);
        if (dist < 10) line[i] += 150 - dist * 10;
    }
    waterfall_->add_line(line);
}

void AudioReceiverView::paint(Painter& painter) {
    frame_counter_++;
    if (frame_counter_ % 3 == 0) {
        update_demo_data();
    }

    View::paint(painter);
}

bool AudioReceiverView::on_key(KeyEvent key) {
    if (View::on_key(key)) return true;

    // If frequency has focus, let it handle Up/Down
    if (frequency_->has_focus()) {
        return frequency_->on_key(key);
    }
    return false;
}

bool AudioReceiverView::on_encoder(int32_t delta) {
    // Always send encoder to frequency
    return frequency_->on_encoder(delta);
}

// ============================================================================
// SpectrumAnalyzerView
// ============================================================================

SpectrumAnalyzerView::SpectrumAnalyzerView(NavigationView& nav)
    : View({0, 16, 240, 304}), nav_(nav)
{
    // Frequency display
    frequency_ = std::make_shared<BigFrequency>(Rect{0, 16, 240, 36}, 100000000);
    add_child(frequency_);

    // Large spectrum display
    spectrum_ = std::make_shared<SpectrumWidget>(Rect{0, 56, 240, 100});
    add_child(spectrum_);

    // Waterfall display
    waterfall_ = std::make_shared<WaterfallWidget>(Rect{0, 160, 240, 100});
    add_child(waterfall_);

    // Span selector
    span_label_ = std::make_shared<Text>(Rect{4, 266, 40, 16}, "Span:");
    span_label_->set_colors(colors::grey, colors::black);
    add_child(span_label_);

    span_ = std::make_shared<OptionsField>(
        Rect{48, 264, 100, 20},
        std::vector<OptionsField::option_t>{
            {"500 kHz", 500000},
            {"1 MHz", 1000000},
            {"2 MHz", 2000000},
            {"5 MHz", 5000000},
            {"10 MHz", 10000000},
        }
    );
    span_->set_selected_index(2);  // Default to 2 MHz
    add_child(span_);
}

void SpectrumAnalyzerView::update_demo_data() {
    spectrum_->generate_demo_data();

    std::vector<uint8_t> line(240);
    for (int i = 0; i < 240; i++) {
        line[i] = 20 + (rand() % 40);
        int dist = std::abs(i - 120);
        if (dist < 15) line[i] += 180 - dist * 10;
        if (std::abs(i - 80) < 8) line[i] += 100;
        if (std::abs(i - 180) < 5) line[i] += 120;
    }
    waterfall_->add_line(line);
}

void SpectrumAnalyzerView::paint(Painter& painter) {
    frame_counter_++;
    if (frame_counter_ % 2 == 0) {
        update_demo_data();
    }

    View::paint(painter);
}

bool SpectrumAnalyzerView::on_encoder(int32_t delta) {
    return frequency_->on_encoder(delta);
}

// ============================================================================
// StatusBar
// ============================================================================

StatusBar::StatusBar() : Widget({0, 0, 240, 16}) {}

void StatusBar::set_title(const std::string& title) {
    title_ = title;
    set_dirty();
}

void StatusBar::set_frequency(int64_t freq_hz) {
    frequency_ = freq_hz;
    set_dirty();
}

void StatusBar::paint(Painter& painter) {
    painter.fill_rect(bounds_, colors::dark_grey);

    // Back button area
    if (back_visible_) {
        painter.draw_text(2, 4, "<", colors::white, colors::dark_grey);
    }

    // Title
    painter.draw_text(16, 4, title_, colors::white, colors::dark_grey);

    // Frequency on right side
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << (frequency_ / 1000000.0) << " MHz";
    std::string freq_str = oss.str();
    int freq_x = 240 - static_cast<int>(freq_str.size()) * 8 - 4;
    painter.draw_text(freq_x, 4, freq_str, colors::green, colors::dark_grey);

    clear_dirty();
}

bool StatusBar::on_touch(const TouchEvent& event) {
    if (event.type == TouchType::Start && back_visible_) {
        // Check if touch is on back button area
        Rect back_area{0, 0, 16, 16};
        if (back_area.contains(event.point.x, event.point.y)) {
            if (on_back) {
                on_back();
            }
            return true;
        }
    }
    return false;
}

// ============================================================================
// SystemView
// ============================================================================

SystemView::SystemView() {
    // Create home view
    auto home = std::make_shared<HomeView>(nav_);
    nav_.push(home);

    // Set up status bar callbacks
    status_bar_.on_back = [this]() {
        if (!nav_.is_top()) {
            nav_.pop();
            if (auto* view = nav_.current_view()) {
                status_bar_.set_title(view->title());
            }
        }
    };

    status_bar_.set_title("Home");
    status_bar_.set_frequency(shim::get_receiver_model().target_frequency());
}

void SystemView::paint(Painter& painter) {
    status_bar_.paint(painter);
    nav_.paint(painter);
}

bool SystemView::on_key(KeyEvent key) {
    if (key == KeyEvent::Back) {
        if (!nav_.is_top()) {
            nav_.pop();
            if (auto* view = nav_.current_view()) {
                status_bar_.set_title(view->title());
            }
            return true;
        }
    }
    return nav_.on_key(key);
}

bool SystemView::on_touch(const TouchEvent& event) {
    if (status_bar_.on_touch(event)) {
        if (auto* view = nav_.current_view()) {
            status_bar_.set_title(view->title());
        }
        return true;
    }
    return nav_.on_touch(event);
}

bool SystemView::on_encoder(int32_t delta) {
    return nav_.on_encoder(delta);
}

// ============================================================================
// Main Demo Entry Point
// ============================================================================

int run_demo_ui() {
    std::cout << "Starting UI Demo..." << std::endl;

    // Create system view
    SystemView system_view;
    Painter painter;

    // Get event dispatcher
    auto& dispatcher = shim::get_event_dispatcher();

    // Set up callbacks
    dispatcher.set_key_callback([&](shim::KeyEvent key, bool pressed) {
        if (!pressed) return;

        auto ui_key = static_cast<KeyEvent>(static_cast<int>(key));
        if (system_view.on_key(ui_key)) {
            system_view.paint(painter);
            shim::get_display().present();
        }
    });

    dispatcher.set_touch_callback([&](const shim::TouchEvent& event) {
        TouchEvent ui_event;
        ui_event.point = {event.x, event.y};
        ui_event.type = static_cast<TouchType>(static_cast<int>(event.type));

        if (system_view.on_touch(ui_event)) {
            system_view.paint(painter);
            shim::get_display().present();
        }
    });

    dispatcher.set_encoder_callback([&](int32_t delta) {
        if (system_view.on_encoder(delta)) {
            system_view.paint(painter);
            shim::get_display().present();
        }
    });

    // Initial paint
    system_view.paint(painter);
    shim::get_display().present();

    std::cout << "UI Demo running. Use arrow keys, Enter, Escape, or mouse." << std::endl;

    // Event loop
    while (!dispatcher.should_stop()) {
        dispatcher.run();
    }

    std::cout << "UI Demo exiting." << std::endl;
    return 0;
}

} // namespace ui_demo

#endif // PORTAPACK_PC_EMULATOR
