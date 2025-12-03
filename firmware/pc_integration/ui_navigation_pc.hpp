/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * PC-compatible NavigationView for running real PortaPack apps.
 */

#ifndef __UI_NAVIGATION_PC_HPP__
#define __UI_NAVIGATION_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "theme.hpp"
#include "message.hpp"
#include "event_m0.hpp"

#include <vector>
#include <memory>
#include <functional>
#include <iostream>

namespace ui {

// Forward declaration
class NavigationView;

// Minimal ViewFactoryBase for PC
class ViewFactoryBase {
public:
    virtual ~ViewFactoryBase() = default;
    virtual std::unique_ptr<View> produce(NavigationView& nav) const = 0;
};

template<typename T>
class ViewFactory : public ViewFactoryBase {
public:
    std::unique_ptr<View> produce(NavigationView& nav) const override {
        return std::make_unique<T>(nav);
    }
};

// App location enum (matching firmware)
enum class app_location_t : uint8_t {
    HOME = 0,
    RX,
    TX,
    TRX,
    UTILITIES,
    GAMES,
    SETTINGS,
    DEBUG_APP,  // Renamed from DEBUG to avoid macro conflict
    EXTERNAL_MODULE
};

// PC-compatible NavigationView
class NavigationView : public View {
public:
    std::function<void(const View&)> on_view_changed{};

    NavigationView() = default;

    // Template for pushing views
    template<class T, class... Args>
    T* push(Args&&... args) {
        return reinterpret_cast<T*>(
            push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...)))
        );
    }

    // Push a pre-created view
    void push(View* v) {
        push_view(std::unique_ptr<View>(v));
    }

    // Push with unique_ptr
    View* push_view(std::unique_ptr<View> new_view) {
        if (!view_stack_.empty()) {
            // Remove current view from widget hierarchy
            auto* current = view_stack_.back().view.get();
            if (current) {
                remove_child(current);
            }
        }

        auto* view_ptr = new_view.get();
        view_stack_.push_back({std::move(new_view), {}});

        // Add new view as child
        add_child(view_ptr);
        view_ptr->set_parent_rect(parent_rect());

        std::cout << "NavigationView: Pushed view, stack size: " << view_stack_.size() << std::endl;

        if (on_view_changed) {
            on_view_changed(*view_ptr);
        }

        set_dirty();
        return view_ptr;
    }

    void pop(bool update = true) {
        if (view_stack_.size() <= 1) {
            return;  // Don't pop the root view
        }

        auto on_pop = view_stack_.back().on_pop;

        // Remove current view
        auto* current = view_stack_.back().view.get();
        if (current) {
            remove_child(current);
        }

        view_stack_.pop_back();

        // Restore previous view
        if (!view_stack_.empty()) {
            auto* prev = view_stack_.back().view.get();
            if (prev) {
                add_child(prev);
            }
        }

        std::cout << "NavigationView: Popped view, stack size: " << view_stack_.size() << std::endl;

        if (on_pop) {
            on_pop();
        }

        if (update) {
            set_dirty();
        }
    }

    void home(bool update = true) {
        while (view_stack_.size() > 1) {
            pop(false);
        }
        if (update) {
            set_dirty();
        }
    }

    bool is_top() const {
        return view_stack_.size() <= 1;
    }

    View* current_view() {
        return view_stack_.empty() ? nullptr : view_stack_.back().view.get();
    }

    // Set callback for when view is popped
    void set_on_pop(std::function<void()> callback) {
        if (!view_stack_.empty()) {
            view_stack_.back().on_pop = callback;
        }
    }

    // Override paint to paint current view
    void paint(Painter& painter) override {
        // Views are children, they'll be painted automatically
    }

    std::string title() const override {
        if (!view_stack_.empty() && view_stack_.back().view) {
            return view_stack_.back().view->title();
        }
        return "Navigation";
    }

private:
    struct ViewState {
        std::unique_ptr<View> view;
        std::function<void()> on_pop;
    };

    std::vector<ViewState> view_stack_;
};

} // namespace ui

#endif // PORTAPACK_PC_EMULATOR
#endif // __UI_NAVIGATION_PC_HPP__
