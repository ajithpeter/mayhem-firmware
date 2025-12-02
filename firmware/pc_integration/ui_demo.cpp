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
    menu_->add_item({"Audio (NFM/AM/WFM)", colors::green, []() {
        std::cout << "Selected: Audio Receiver" << std::endl;
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
    menu_->add_item({"Spectrum", colors::green, []() {
        std::cout << "Selected: Spectrum Analyzer" << std::endl;
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
