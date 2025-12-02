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

#ifndef __UI_DEMO_HPP__
#define __UI_DEMO_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ui_demo {

/**
 * @brief Simple color structure for UI rendering.
 */
struct Color {
    uint8_t r, g, b;

    Color() : r(0), g(0), b(0) {}
    Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

    // Convert to RGB565 for display
    uint16_t to_rgb565() const {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};

// Standard colors
namespace colors {
    const Color black{0, 0, 0};
    const Color white{255, 255, 255};
    const Color red{255, 0, 0};
    const Color green{0, 255, 0};
    const Color blue{0, 0, 255};
    const Color yellow{255, 255, 0};
    const Color cyan{0, 255, 255};
    const Color magenta{255, 0, 255};
    const Color dark_blue{0, 0, 64};
    const Color dark_grey{32, 32, 32};
    const Color grey{128, 128, 128};
    const Color light_grey{192, 192, 192};
}

/**
 * @brief Rectangle structure for UI layout.
 */
struct Rect {
    int16_t x, y;
    int16_t w, h;

    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int16_t x_, int16_t y_, int16_t w_, int16_t h_)
        : x(x_), y(y_), w(w_), h(h_) {}

    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

/**
 * @brief Point structure.
 */
struct Point {
    int16_t x, y;
    Point() : x(0), y(0) {}
    Point(int16_t x_, int16_t y_) : x(x_), y(y_) {}
};

/**
 * @brief Key events matching PortaPack.
 */
enum class KeyEvent : uint8_t {
    Right = 0,
    Left = 1,
    Down = 2,
    Up = 3,
    Select = 4,
    Dfu = 5,
    Back = 6,
};

/**
 * @brief Touch event types.
 */
enum class TouchType {
    Start,
    Move,
    End
};

/**
 * @brief Touch event structure.
 */
struct TouchEvent {
    Point point;
    TouchType type;
};

// Forward declaration
class Widget;
class View;

/**
 * @brief Simple painter for drawing to display.
 */
class Painter {
public:
    void fill_rect(const Rect& rect, const Color& color);
    void draw_rect(const Rect& rect, const Color& color);
    void draw_hline(int x, int y, int w, const Color& color);
    void draw_vline(int x, int y, int h, const Color& color);
    void draw_text(int x, int y, const std::string& text, const Color& fg, const Color& bg);
    void draw_char(int x, int y, char c, const Color& fg, const Color& bg);
};

/**
 * @brief Base widget class.
 */
class Widget {
public:
    Widget(const Rect& rect) : bounds_(rect) {}
    virtual ~Widget() = default;

    virtual void paint(Painter& painter) = 0;
    virtual bool on_key(KeyEvent key) { (void)key; return false; }
    virtual bool on_touch(const TouchEvent& event) { (void)event; return false; }
    virtual bool on_encoder(int32_t delta) { (void)delta; return false; }

    const Rect& bounds() const { return bounds_; }
    void set_bounds(const Rect& rect) { bounds_ = rect; dirty_ = true; }

    bool is_dirty() const { return dirty_; }
    void set_dirty() { dirty_ = true; }
    void clear_dirty() { dirty_ = false; }

    bool has_focus() const { return focused_; }
    virtual void focus() { focused_ = true; set_dirty(); }
    virtual void blur() { focused_ = false; set_dirty(); }

protected:
    Rect bounds_;
    bool dirty_ = true;
    bool focused_ = false;
};

/**
 * @brief Simple text label widget.
 */
class Text : public Widget {
public:
    Text(const Rect& rect, const std::string& text = "")
        : Widget(rect), text_(text), fg_(colors::white), bg_(colors::black) {}

    void set_text(const std::string& text) { text_ = text; set_dirty(); }
    void set_colors(const Color& fg, const Color& bg) { fg_ = fg; bg_ = bg; set_dirty(); }

    void paint(Painter& painter) override;

private:
    std::string text_;
    Color fg_, bg_;
};

/**
 * @brief Simple button widget.
 */
class Button : public Widget {
public:
    std::function<void()> on_select;

    Button(const Rect& rect, const std::string& label = "")
        : Widget(rect), label_(label) {}

    void set_label(const std::string& label) { label_ = label; set_dirty(); }

    void paint(Painter& painter) override;
    bool on_key(KeyEvent key) override;
    bool on_touch(const TouchEvent& event) override;

private:
    std::string label_;
};

/**
 * @brief Menu item for menu views.
 */
struct MenuItem {
    std::string label;
    Color color;
    std::function<void()> on_select;

    MenuItem(const std::string& lbl, const Color& clr, std::function<void()> callback)
        : label(lbl), color(clr), on_select(callback) {}
};

/**
 * @brief Simple menu view with selectable items.
 */
class MenuView : public Widget {
public:
    MenuView(const Rect& rect);

    void add_item(const MenuItem& item);
    void clear_items();

    void paint(Painter& painter) override;
    bool on_key(KeyEvent key) override;
    bool on_touch(const TouchEvent& event) override;
    bool on_encoder(int32_t delta) override;

private:
    std::vector<MenuItem> items_;
    size_t selected_index_ = 0;
    int scroll_offset_ = 0;
    static constexpr int ITEM_HEIGHT = 24;
    static constexpr int VISIBLE_ITEMS = 10;

    void select_item();
    void ensure_visible();
};

/**
 * @brief Base view class (container for widgets).
 */
class View : public Widget {
public:
    View(const Rect& rect) : Widget(rect) {}

    void add_child(std::shared_ptr<Widget> child);
    void remove_all_children();

    void paint(Painter& painter) override;
    bool on_key(KeyEvent key) override;
    bool on_touch(const TouchEvent& event) override;
    bool on_encoder(int32_t delta) override;

    virtual std::string title() const { return "View"; }

protected:
    std::vector<std::shared_ptr<Widget>> children_;
    size_t focused_child_ = 0;

    void focus_next();
    void focus_prev();
};

/**
 * @brief Navigation view managing view stack.
 */
class NavigationView {
public:
    NavigationView();

    void push(std::shared_ptr<View> view);
    void pop();
    void home();

    View* current_view();
    bool is_top() const { return view_stack_.size() <= 1; }

    void paint(Painter& painter);
    bool on_key(KeyEvent key);
    bool on_touch(const TouchEvent& event);
    bool on_encoder(int32_t delta);

private:
    std::vector<std::shared_ptr<View>> view_stack_;
};

/**
 * @brief Home menu view.
 */
class HomeView : public View {
public:
    HomeView(NavigationView& nav);
    std::string title() const override { return "Home"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<MenuView> menu_;
};

/**
 * @brief Receive menu view.
 */
class ReceiveMenuView : public View {
public:
    ReceiveMenuView(NavigationView& nav);
    std::string title() const override { return "Receive"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<MenuView> menu_;
};

/**
 * @brief Transmit menu view.
 */
class TransmitMenuView : public View {
public:
    TransmitMenuView(NavigationView& nav);
    std::string title() const override { return "Transmit"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<MenuView> menu_;
};

/**
 * @brief Utilities menu view.
 */
class UtilitiesMenuView : public View {
public:
    UtilitiesMenuView(NavigationView& nav);
    std::string title() const override { return "Utilities"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<MenuView> menu_;
};

/**
 * @brief Settings menu view.
 */
class SettingsMenuView : public View {
public:
    SettingsMenuView(NavigationView& nav);
    std::string title() const override { return "Settings"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<MenuView> menu_;
};

/**
 * @brief About/Info view.
 */
class AboutView : public View {
public:
    AboutView(NavigationView& nav);
    std::string title() const override { return "About"; }

private:
    NavigationView& nav_;
    std::shared_ptr<Text> title_text_;
    std::shared_ptr<Text> version_text_;
    std::shared_ptr<Text> info_text_;
};

/**
 * @brief Status bar at top of screen.
 */
class StatusBar : public Widget {
public:
    StatusBar();

    void set_title(const std::string& title);
    void set_frequency(int64_t freq_hz);
    void set_back_visible(bool visible) { back_visible_ = visible; set_dirty(); }

    void paint(Painter& painter) override;
    bool on_touch(const TouchEvent& event) override;

    std::function<void()> on_back;

private:
    std::string title_ = "PortaPack";
    int64_t frequency_ = 100000000;
    bool back_visible_ = true;
};

/**
 * @brief Main system view containing navigation and status bar.
 */
class SystemView {
public:
    SystemView();

    void paint(Painter& painter);
    bool on_key(KeyEvent key);
    bool on_touch(const TouchEvent& event);
    bool on_encoder(int32_t delta);

    NavigationView& navigation() { return nav_; }
    StatusBar& status_bar() { return status_bar_; }

private:
    StatusBar status_bar_;
    NavigationView nav_;
};

/**
 * @brief Run the demo UI.
 * @return Exit code
 */
int run_demo_ui();

} // namespace ui_demo

#endif // PORTAPACK_PC_EMULATOR

#endif // __UI_DEMO_HPP__
