# Portapack Mayhem PC Emulator - Shim Layer Architecture

## Design Philosophy

**Goal**: Create a transparent HAL shim layer that intercepts existing Mayhem API calls and redirects them to PC implementations, ensuring:

1. **Zero application code changes** - Apps compile unmodified
2. **Forward compatibility** - Future Mayhem updates work automatically
3. **External app support** - Third-party apps work without modification
4. **Clean separation** - Embedded vs PC code isolated via compile-time switches

---

## 1. Build System Architecture

### 1.1 Conditional Compilation Strategy

```cmake
# Top-level CMakeLists.txt
option(PORTAPACK_TARGET "Build target" "EMBEDDED")
# Options: EMBEDDED, PC_EMULATOR

if(PORTAPACK_TARGET STREQUAL "PC_EMULATOR")
    add_definitions(-DPORTAPACK_PC_EMULATOR=1)
    set(HAL_DIR "${CMAKE_SOURCE_DIR}/hal/pc")
else()
    add_definitions(-DPORTAPACK_EMBEDDED=1)
    set(HAL_DIR "${CMAKE_SOURCE_DIR}/hal/lpc43xx")
endif()

# Include HAL implementation
add_subdirectory(${HAL_DIR})
```

### 1.2 Directory Structure

```
firmware/
├── application/           # UNCHANGED - All apps compile as-is
├── baseband/             # UNCHANGED - All processors compile as-is
├── common/               # UNCHANGED - Shared definitions
│
├── hal/                  # NEW - Hardware Abstraction Layer
│   ├── interface/        # Pure virtual interfaces (shared)
│   │   ├── hal_display.hpp
│   │   ├── hal_audio.hpp
│   │   ├── hal_rf.hpp
│   │   ├── hal_input.hpp
│   │   ├── hal_storage.hpp
│   │   └── hal_time.hpp
│   │
│   ├── lpc43xx/          # Original embedded implementations
│   │   ├── display_ili9341.cpp
│   │   ├── audio_wm8731.cpp
│   │   ├── rf_max2837.cpp
│   │   ├── input_gpio.cpp
│   │   ├── storage_sdio.cpp
│   │   └── time_rtc.cpp
│   │
│   └── pc/               # PC emulator implementations
│       ├── display_sdl.cpp
│       ├── audio_portaudio.cpp
│       ├── rf_soapysdr.cpp
│       ├── input_sdl.cpp
│       ├── storage_filesystem.cpp
│       └── time_system.cpp
│
└── shim/                 # NEW - API shim wrappers
    ├── portapack_shim.cpp
    ├── baseband_shim.cpp
    ├── audio_shim.cpp
    ├── receiver_model_shim.cpp
    ├── transmitter_model_shim.cpp
    └── shared_memory_shim.cpp
```

---

## 2. Core Shim Interfaces

### 2.1 Display Shim (`portapack::display`)

**Original API** (from `firmware/application/portapack.hpp`):

```cpp
namespace portapack {
    extern Display display;
}

class Display {
public:
    void fill_rectangle(Rect rect, Color color);
    void draw_pixel(Point p, Color color);
    void draw_line(Point start, Point end, Color color);
    void draw_rectangle(Rect rect, Color color);
    void draw_bitmap(Point p, const Bitmap& bitmap, Color fg, Color bg);
    void drawBMP(Point p, const uint8_t* bmp, bool transparency);
    void render_line(Point p, uint8_t count, const Color* colors);
    void scroll_set_area(uint16_t top, uint16_t bottom);
    void scroll(int16_t delta);
    
    constexpr int width() { return 240; }
    constexpr int height() { return 320; }
};
```

**Shim Implementation**:

```cpp
// shim/display_shim.cpp
#include "portapack.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/display_sdl.hpp"
static DisplaySDL display_impl;
#else
#include "hal/lpc43xx/display_ili9341.hpp"
static DisplayILI9341 display_impl;
#endif

namespace portapack {

Display display;  // Global instance apps expect

void Display::fill_rectangle(Rect rect, Color color) {
    display_impl.fill_rectangle(rect.left(), rect.top(), 
                                 rect.width(), rect.height(),
                                 color.v);
}

void Display::draw_pixel(Point p, Color color) {
    display_impl.draw_pixel(p.x(), p.y(), color.v);
}

void Display::draw_bitmap(Point p, const Bitmap& bitmap, Color fg, Color bg) {
    display_impl.draw_bitmap(p.x(), p.y(), 
                              bitmap.data, bitmap.size.width(), bitmap.size.height(),
                              fg.v, bg.v);
}

void Display::render_line(Point p, uint8_t count, const Color* colors) {
    display_impl.render_line(p.x(), p.y(), count, 
                              reinterpret_cast<const uint16_t*>(colors));
}

// ... remaining methods

} // namespace portapack
```

**PC HAL Implementation**:

```cpp
// hal/pc/display_sdl.hpp
#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <mutex>

class DisplaySDL {
public:
    static constexpr int WIDTH = 240;
    static constexpr int HEIGHT = 320;
    static constexpr int SCALE = 2;  // 2x scaling for visibility
    
    DisplaySDL();
    ~DisplaySDL();
    
    void fill_rectangle(int x, int y, int w, int h, uint16_t color);
    void draw_pixel(int x, int y, uint16_t color);
    void draw_bitmap(int x, int y, const uint8_t* data, int w, int h, 
                     uint16_t fg, uint16_t bg);
    void render_line(int x, int y, int count, const uint16_t* colors);
    void scroll_set_area(uint16_t top, uint16_t bottom);
    void scroll(int16_t delta);
    
    void present();  // Called by main loop to update screen
    
private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* framebuffer_ = nullptr;
    uint16_t pixels_[WIDTH * HEIGHT];  // RGB565 framebuffer
    std::mutex mutex_;
    
    uint16_t scroll_top_ = 0;
    uint16_t scroll_bottom_ = HEIGHT;
    int16_t scroll_offset_ = 0;
    
    // RGB565 to ARGB8888 conversion for SDL
    static uint32_t rgb565_to_argb8888(uint16_t c) {
        uint8_t r = ((c >> 11) & 0x1F) << 3;
        uint8_t g = ((c >> 5) & 0x3F) << 2;
        uint8_t b = (c & 0x1F) << 3;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }
};
```

---

### 2.2 ReceiverModel Shim

**Original API** (from `firmware/application/receiver_model.hpp`):

```cpp
class ReceiverModel {
public:
    rf::Frequency target_frequency() const;
    void set_target_frequency(rf::Frequency f);
    
    uint32_t baseband_bandwidth() const;
    void set_baseband_bandwidth(uint32_t v);
    
    uint32_t sampling_rate() const;
    void set_sampling_rate(uint32_t v);
    
    int32_t lna() const;
    void set_lna(int32_t v_db);
    
    int32_t vga() const;
    void set_vga(int32_t v_db);
    
    bool rf_amp() const;
    void set_rf_amp(bool v);
    
    Mode modulation() const;
    void set_modulation(Mode v);
    
    void enable();
    void disable();
    
    enum class Mode {
        AMAudio,
        NarrowbandFMAudio,
        WidebandFMAudio,
        SpectrumAnalysis,
        Capture,
        // ...
    };
};

extern ReceiverModel receiver_model;  // Global instance
```

**Shim Implementation**:

```cpp
// shim/receiver_model_shim.cpp
#include "receiver_model.hpp"
#include "baseband_api.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/rf_soapysdr.hpp"
extern RFSoapySDR rf_hal;  // Global RF HAL instance
#endif

ReceiverModel receiver_model;  // Global instance

void ReceiverModel::set_target_frequency(rf::Frequency f) {
    target_frequency_ = f;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_frequency(static_cast<double>(f));
#else
    // Original embedded code path
    radio::set_tuning_frequency(f);
#endif
}

void ReceiverModel::set_sampling_rate(uint32_t v) {
    sampling_rate_ = v;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_sample_rate(static_cast<double>(v));
#else
    update_tuning_frequency();
    baseband::set_sample_rate(v);
#endif
}

void ReceiverModel::set_baseband_bandwidth(uint32_t v) {
    baseband_bandwidth_ = v;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_bandwidth(static_cast<double>(v));
#else
    radio::set_baseband_filter_bandwidth(v);
#endif
}

void ReceiverModel::set_lna(int32_t v_db) {
    lna_gain_db_ = v_db;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_gain("LNA", static_cast<double>(v_db));
#else
    radio::set_lna_gain(v_db);
#endif
}

void ReceiverModel::set_vga(int32_t v_db) {
    vga_gain_db_ = v_db;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_gain("VGA", static_cast<double>(v_db));
#else
    radio::set_vga_gain(v_db);
#endif
}

void ReceiverModel::set_rf_amp(bool v) {
    rf_amp_ = v;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_amp_enable(v);
#else
    radio::set_rf_amp(v);
#endif
}

void ReceiverModel::enable() {
    enabled_ = true;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.start_rx();
#else
    radio::set_direction(rf::Direction::Receive);
    update_modulation();
#endif
}

void ReceiverModel::disable() {
    enabled_ = false;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.stop();
#else
    baseband::stop();
    radio::disable();
#endif
}
```

---

### 2.3 TransmitterModel Shim

**Original API** (from `firmware/application/transmitter_model.hpp`):

```cpp
class TransmitterModel {
public:
    rf::Frequency target_frequency() const;
    void set_target_frequency(rf::Frequency f);
    
    uint32_t baseband_bandwidth() const;
    void set_baseband_bandwidth(uint32_t v);
    
    uint32_t sampling_rate() const;
    void set_sampling_rate(uint32_t v);
    
    int32_t tx_gain() const;
    void set_tx_gain(int32_t v_db);
    
    bool rf_amp() const;
    void set_rf_amp(bool v);
    
    void enable();
    void disable();
};

extern TransmitterModel transmitter_model;
```

**Shim Implementation**:

```cpp
// shim/transmitter_model_shim.cpp
#include "transmitter_model.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/rf_soapysdr.hpp"
extern RFSoapySDR rf_hal;
#endif

TransmitterModel transmitter_model;

void TransmitterModel::set_target_frequency(rf::Frequency f) {
    target_frequency_ = f;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.set_frequency(static_cast<double>(f));
#else
    portapack::persistent_memory::set_tuned_frequency(f);
#endif
}

void TransmitterModel::enable() {
    enabled_ = true;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.start_tx();
#else
    radio::set_direction(rf::Direction::Transmit);
    radio::enable({...});
#endif
}

void TransmitterModel::disable() {
    enabled_ = false;
    
#ifdef PORTAPACK_PC_EMULATOR
    rf_hal.stop();
#else
    baseband::stop();
    radio::disable();
#endif
}
```

---

### 2.4 Baseband API Shim

**Original API** (from `firmware/application/baseband_api.hpp`):

```cpp
namespace baseband {
    void run_image(spi_flash::image_tag_t tag);
    void shutdown();
    
    void set_sample_rate(uint32_t sample_rate);
    
    // Mode-specific configuration
    void set_afsk(uint32_t baudrate, uint32_t word_length, 
                  uint32_t trigger_value, bool trigger_word);
    void set_ook_data(uint32_t bitstream_length, uint32_t samples_per_bit,
                      uint8_t repeat, uint32_t pause_symbols);
    void set_fsk(uint32_t symbol_rate);
    void set_am_audio(uint32_t bandwidth);
    void set_nfm_audio(uint32_t bandwidth, uint32_t squelch);
    void set_wfm_audio();
    
    void spectrum_streaming_start();
    void spectrum_streaming_stop();
    
    void capture_start(CaptureConfig* config);
    void capture_stop();
    
    void replay_start(ReplayConfig* config);
    void replay_stop();
}
```

**Shim Implementation**:

```cpp
// shim/baseband_shim.cpp
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/baseband_emulator.hpp"
extern BasebandEmulator baseband_emu;
#endif

namespace baseband {

void run_image(spi_flash::image_tag_t tag) {
#ifdef PORTAPACK_PC_EMULATOR
    // Load the appropriate processor class based on tag
    baseband_emu.load_processor(tag);
#else
    // Original: Copy image to M4 RAM and reset M4
    m4_request_shutdown();
    m4_load_image(tag);
    m4_init();
#endif
}

void shutdown() {
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.stop();
#else
    m4_request_shutdown();
#endif
}

void set_sample_rate(uint32_t sample_rate) {
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.set_sample_rate(sample_rate);
#else
    // Send message to M4
    BasebandConfigurationMessage message { sample_rate, ... };
    shared_memory.baseband_queue.push(message);
#endif
}

void set_afsk(uint32_t baudrate, uint32_t word_length,
              uint32_t trigger_value, bool trigger_word) {
    AFSKRxConfigureMessage message {
        baudrate, word_length, trigger_value, trigger_word
    };
    
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.on_message(&message);
#else
    shared_memory.baseband_queue.push(message);
#endif
}

void set_ook_data(uint32_t bitstream_length, uint32_t samples_per_bit,
                  uint8_t repeat, uint32_t pause_symbols) {
    OOKConfigureMessage message {
        bitstream_length, samples_per_bit, repeat, pause_symbols
    };
    
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.on_message(&message);
#else
    shared_memory.baseband_queue.push(message);
#endif
}

// ... similar patterns for other mode configurations

void spectrum_streaming_start() {
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.spectrum_start();
#else
    SpectrumStreamingConfigMessage message { true };
    shared_memory.baseband_queue.push(message);
#endif
}

void capture_start(CaptureConfig* config) {
#ifdef PORTAPACK_PC_EMULATOR
    baseband_emu.capture_start(config);
#else
    CaptureConfigMessage message { config };
    shared_memory.baseband_queue.push(message);
#endif
}

} // namespace baseband
```

---

### 2.5 Shared Memory Shim

**Original API** (from `firmware/common/portapack_shared_memory.hpp`):

```cpp
struct SharedMemory {
    MessageQueue<Message, 16> baseband_queue;      // M0 → M4
    MessageQueue<Message, 16> application_queue;   // M4 → M0
    
    std::array<uint8_t, 32768> bb_data;  // TX bitstreams, etc.
    
    volatile uint32_t m4_state;
    volatile uint32_t application_counter;
    volatile uint32_t baseband_counter;
    
    // Spectrum data
    ChannelSpectrum spectrum;
};

extern SharedMemory& shared_memory;
```

**Shim Implementation**:

```cpp
// shim/shared_memory_shim.cpp
#include "portapack_shared_memory.hpp"

#ifdef PORTAPACK_PC_EMULATOR

#include <mutex>
#include <condition_variable>
#include <queue>

// Thread-safe message queue for PC
template<typename T, size_t N>
class ThreadSafeMessageQueue {
public:
    bool push(const T& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= N) return false;
        queue_.push(message);
        cv_.notify_one();
        return true;
    }
    
    bool pop(T& message, uint32_t timeout_ms = 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (timeout_ms > 0) {
            if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              [this]{ return !queue_.empty(); })) {
                return false;
            }
        } else if (queue_.empty()) {
            return false;
        }
        message = queue_.front();
        queue_.pop();
        return true;
    }
    
    bool is_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

// PC implementation of SharedMemory
struct SharedMemoryPC {
    ThreadSafeMessageQueue<Message, 16> baseband_queue;
    ThreadSafeMessageQueue<Message, 16> application_queue;
    
    std::array<uint8_t, 32768> bb_data;
    
    std::atomic<uint32_t> m4_state{0};
    std::atomic<uint32_t> application_counter{0};
    std::atomic<uint32_t> baseband_counter{0};
    
    ChannelSpectrum spectrum;
    std::mutex spectrum_mutex;
};

static SharedMemoryPC shared_memory_pc;

// Provide the expected global reference
SharedMemory& shared_memory = reinterpret_cast<SharedMemory&>(shared_memory_pc);

#else

// Original embedded: shared memory at fixed address
SharedMemory& shared_memory = *reinterpret_cast<SharedMemory*>(0x10088000);

#endif
```

---

### 2.6 Audio API Shim

**Original API** (from `firmware/application/audio.hpp`):

```cpp
namespace audio {
    namespace output {
        void start();
        void stop();
    }
    
    namespace input {
        void start();
        void stop();
    }
    
    enum class Rate : uint32_t {
        Hz_12000 = 12000,
        Hz_24000 = 24000,
        Hz_48000 = 48000,
    };
    
    void set_rate(Rate rate);
    void headphone_volume(int8_t volume);
}
```

**Shim Implementation**:

```cpp
// shim/audio_shim.cpp
#include "audio.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/audio_portaudio.hpp"
extern AudioPortAudio audio_hal;
#endif

namespace audio {

void set_rate(Rate rate) {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.set_sample_rate(static_cast<uint32_t>(rate));
#else
    // Configure WM8731/AK4951 codec
    codec.set_sample_rate(static_cast<uint32_t>(rate));
#endif
}

void headphone_volume(int8_t volume) {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.set_volume(volume);
#else
    codec.headphone_volume(volume);
#endif
}

namespace output {

void start() {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.output_start();
#else
    i2s::i2s0::tx_start();
    codec.output_start();
#endif
}

void stop() {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.output_stop();
#else
    codec.output_stop();
    i2s::i2s0::tx_stop();
#endif
}

} // namespace output

namespace input {

void start() {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.input_start();
#else
    codec.input_start();
    i2s::i2s0::rx_start();
#endif
}

void stop() {
#ifdef PORTAPACK_PC_EMULATOR
    audio_hal.input_stop();
#else
    i2s::i2s0::rx_stop();
    codec.input_stop();
#endif
}

} // namespace input
} // namespace audio
```

---

### 2.7 Input/Events Shim

**Original API** (from `firmware/application/event_m0.cpp`):

```cpp
class EventDispatcher {
public:
    void run();
    void request_stop();
    
    static void events_flag(uint32_t flags);
    
    static constexpr uint32_t EVT_MASK_SWITCHES = 1 << 0;
    static constexpr uint32_t EVT_MASK_ENCODER = 1 << 1;
    static constexpr uint32_t EVT_MASK_TOUCH = 1 << 2;
    static constexpr uint32_t EVT_MASK_RTC_TICK = 1 << 3;
    static constexpr uint32_t EVT_MASK_APPLICATION = 1 << 4;
};

namespace ui {
    enum class KeyEvent {
        Select,
        Back,
        Left, Right, Up, Down,
        Touch,
    };
}
```

**Shim Implementation**:

```cpp
// shim/event_shim.cpp
#include "event_m0.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include "hal/pc/input_sdl.hpp"
#include <SDL2/SDL.h>

void EventDispatcher::run() {
    while (!should_stop_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    request_stop();
                    break;
                    
                case SDL_KEYDOWN:
                    handle_key(event.key.keysym.sym, true);
                    break;
                    
                case SDL_KEYUP:
                    handle_key(event.key.keysym.sym, false);
                    break;
                    
                case SDL_MOUSEWHEEL:
                    handle_encoder(event.wheel.y);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    handle_touch(event.button.x, event.button.y,
                                event.type == SDL_MOUSEBUTTONDOWN);
                    break;
            }
        }
        
        // Process message queues
        dispatch_messages();
        
        // Update display
        display_impl.present();
        
        // Rate limit to ~60fps
        SDL_Delay(16);
    }
}

void EventDispatcher::handle_key(SDL_Keycode key, bool pressed) {
    ui::KeyEvent ui_event;
    
    switch (key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            ui_event = ui::KeyEvent::Select;
            break;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
            ui_event = ui::KeyEvent::Back;
            break;
        case SDLK_LEFT:
            ui_event = ui::KeyEvent::Left;
            break;
        case SDLK_RIGHT:
            ui_event = ui::KeyEvent::Right;
            break;
        case SDLK_UP:
            ui_event = ui::KeyEvent::Up;
            break;
        case SDLK_DOWN:
            ui_event = ui::KeyEvent::Down;
            break;
        default:
            return;
    }
    
    if (pressed) {
        on_key_press(ui_event);
    }
}

void EventDispatcher::handle_encoder(int delta) {
    // Encoder rotation maps to scroll wheel
    encoder_position_ += delta;
    events_flag(EVT_MASK_ENCODER);
}

void EventDispatcher::handle_touch(int x, int y, bool pressed) {
    // Scale from window to display coordinates
    touch_point_ = { 
        static_cast<int16_t>(x / DisplaySDL::SCALE),
        static_cast<int16_t>(y / DisplaySDL::SCALE)
    };
    touch_pressed_ = pressed;
    events_flag(EVT_MASK_TOUCH);
}

#else
// Original embedded implementation unchanged
#endif
```

---

### 2.8 Persistent Memory Shim

**Original API** (from `firmware/common/portapack_persistent_memory.hpp`):

```cpp
namespace portapack {
namespace persistent_memory {
    rf::Frequency tuned_frequency();
    void set_tuned_frequency(rf::Frequency f);
    
    int32_t volume_db();
    void set_volume_db(int32_t v);
    
    uint32_t modem_baudrate();
    void set_modem_baudrate(uint32_t v);
    
    serial_format_t serial_format();
    void set_serial_format(serial_format_t v);
    
    // ... many more settings
}}
```

**Shim Implementation**:

```cpp
// shim/persistent_memory_shim.cpp
#include "portapack_persistent_memory.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include <fstream>
#include <nlohmann/json.hpp>

// PC: Store settings in JSON file
static nlohmann::json settings;
static const std::string SETTINGS_FILE = "portapack_settings.json";

static void load_settings() {
    std::ifstream f(SETTINGS_FILE);
    if (f.good()) {
        f >> settings;
    }
}

static void save_settings() {
    std::ofstream f(SETTINGS_FILE);
    f << settings.dump(2);
}

namespace portapack {
namespace persistent_memory {

rf::Frequency tuned_frequency() {
    return settings.value("tuned_frequency", 100000000);
}

void set_tuned_frequency(rf::Frequency f) {
    settings["tuned_frequency"] = f;
    save_settings();
}

int32_t volume_db() {
    return settings.value("volume_db", -20);
}

void set_volume_db(int32_t v) {
    settings["volume_db"] = v;
    save_settings();
}

// ... similar for all settings

}}

#else

// Original embedded: Use VBAT-backed RAM at 0x40041000
namespace portapack {
namespace persistent_memory {

static PersistentMemory* const memory = 
    reinterpret_cast<PersistentMemory*>(0x40041000);

rf::Frequency tuned_frequency() {
    return memory->tuned_frequency;
}

void set_tuned_frequency(rf::Frequency f) {
    memory->tuned_frequency = f;
}

// ...

}}

#endif
```

---

### 2.9 SD Card / File I/O Shim

**Original API** (from `firmware/application/file.hpp`):

```cpp
class File {
public:
    Optional<Error> open(const std::filesystem::path& path, 
                         bool read, bool write);
    Optional<Error> close();
    
    Result<size_t> read(void* buffer, size_t count);
    Result<size_t> write(const void* buffer, size_t count);
    
    Result<uint64_t> seek(uint64_t offset);
    Result<uint64_t> size();
    
    static Result<std::vector<std::filesystem::path>> 
        list_directory(const std::filesystem::path& path);
};
```

**Shim Implementation**:

```cpp
// shim/file_shim.cpp
#include "file.hpp"

#ifdef PORTAPACK_PC_EMULATOR
#include <fstream>
#include <filesystem>

// PC: Use standard filesystem with configurable SD card root
static std::filesystem::path sd_root = "./sdcard";

class FilePC {
public:
    Optional<Error> open(const std::filesystem::path& path,
                         bool read, bool write) {
        auto full_path = sd_root / path;
        
        std::ios::openmode mode = std::ios::binary;
        if (read) mode |= std::ios::in;
        if (write) mode |= std::ios::out;
        
        stream_.open(full_path, mode);
        if (!stream_.is_open()) {
            return Error::ACCESS_DENIED;
        }
        return {};
    }
    
    Result<size_t> read(void* buffer, size_t count) {
        stream_.read(static_cast<char*>(buffer), count);
        return stream_.gcount();
    }
    
    Result<size_t> write(const void* buffer, size_t count) {
        stream_.write(static_cast<const char*>(buffer), count);
        return count;
    }
    
    // ...
    
private:
    std::fstream stream_;
};

#else

// Original: Use FatFs on SD card via SDIO
class FileEmbedded {
    FIL f;
    // ...
};

#endif
```

---

## 3. PC HAL Implementations

### 3.1 SoapySDR RF HAL

```cpp
// hal/pc/rf_soapysdr.hpp
#pragma once
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <atomic>
#include <thread>
#include <functional>

class RFSoapySDR {
public:
    using RxCallback = std::function<void(const int8_t*, size_t)>;
    using TxCallback = std::function<size_t(int8_t*, size_t)>;
    
    RFSoapySDR();
    ~RFSoapySDR();
    
    bool init(const std::string& device_args = "");
    void close();
    
    // Configuration
    void set_frequency(double freq_hz);
    void set_sample_rate(double rate);
    void set_bandwidth(double bw);
    void set_gain(const std::string& name, double gain_db);
    void set_amp_enable(bool enable);
    
    // Streaming
    void start_rx();
    void start_tx();
    void stop();
    
    void set_rx_callback(RxCallback cb) { rx_callback_ = cb; }
    void set_tx_callback(TxCallback cb) { tx_callback_ = cb; }
    
    // Direct buffer access (for compatibility with baseband thread)
    int read_samples(int8_t* buffer, size_t count);
    int write_samples(const int8_t* buffer, size_t count);
    
private:
    void rx_thread_func();
    void tx_thread_func();
    
    SoapySDR::Device* device_ = nullptr;
    SoapySDR::Stream* rx_stream_ = nullptr;
    SoapySDR::Stream* tx_stream_ = nullptr;
    
    std::thread rx_thread_;
    std::thread tx_thread_;
    std::atomic<bool> running_{false};
    
    RxCallback rx_callback_;
    TxCallback tx_callback_;
    
    double frequency_ = 100e6;
    double sample_rate_ = 2.4e6;
    double bandwidth_ = 1.75e6;
};
```

**Implementation**:

```cpp
// hal/pc/rf_soapysdr.cpp
#include "rf_soapysdr.hpp"
#include <iostream>

RFSoapySDR::RFSoapySDR() = default;

RFSoapySDR::~RFSoapySDR() {
    close();
}

bool RFSoapySDR::init(const std::string& device_args) {
    // Find device
    auto results = SoapySDR::Device::enumerate(device_args);
    if (results.empty()) {
        std::cerr << "No SoapySDR devices found" << std::endl;
        return false;
    }
    
    device_ = SoapySDR::Device::make(results[0]);
    if (!device_) {
        std::cerr << "Failed to create SoapySDR device" << std::endl;
        return false;
    }
    
    // Apply initial configuration
    set_frequency(frequency_);
    set_sample_rate(sample_rate_);
    set_bandwidth(bandwidth_);
    
    return true;
}

void RFSoapySDR::close() {
    stop();
    if (device_) {
        SoapySDR::Device::unmake(device_);
        device_ = nullptr;
    }
}

void RFSoapySDR::set_frequency(double freq_hz) {
    frequency_ = freq_hz;
    if (device_) {
        device_->setFrequency(SOAPY_SDR_RX, 0, freq_hz);
        device_->setFrequency(SOAPY_SDR_TX, 0, freq_hz);
    }
}

void RFSoapySDR::set_sample_rate(double rate) {
    sample_rate_ = rate;
    if (device_) {
        device_->setSampleRate(SOAPY_SDR_RX, 0, rate);
        device_->setSampleRate(SOAPY_SDR_TX, 0, rate);
    }
}

void RFSoapySDR::start_rx() {
    if (!device_ || running_) return;
    
    // Setup RX stream - CS8 format matches HackRF native
    rx_stream_ = device_->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CS8);
    device_->activateStream(rx_stream_);
    
    running_ = true;
    rx_thread_ = std::thread(&RFSoapySDR::rx_thread_func, this);
}

void RFSoapySDR::stop() {
    running_ = false;
    
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    if (tx_thread_.joinable()) {
        tx_thread_.join();
    }
    
    if (rx_stream_) {
        device_->deactivateStream(rx_stream_);
        device_->closeStream(rx_stream_);
        rx_stream_ = nullptr;
    }
    if (tx_stream_) {
        device_->deactivateStream(tx_stream_);
        device_->closeStream(tx_stream_);
        tx_stream_ = nullptr;
    }
}

void RFSoapySDR::rx_thread_func() {
    constexpr size_t BUFFER_SIZE = 2048;  // Match Mayhem buffer size
    int8_t buffer[BUFFER_SIZE * 2];  // IQ pairs
    
    while (running_) {
        void* buffs[] = { buffer };
        int flags;
        long long time_ns;
        
        int ret = device_->readStream(rx_stream_, buffs, BUFFER_SIZE,
                                       flags, time_ns, 100000);
        
        if (ret > 0 && rx_callback_) {
            rx_callback_(buffer, ret);
        }
    }
}

int RFSoapySDR::read_samples(int8_t* buffer, size_t count) {
    if (!rx_stream_) return -1;
    
    void* buffs[] = { buffer };
    int flags;
    long long time_ns;
    
    return device_->readStream(rx_stream_, buffs, count, 
                                flags, time_ns, 100000);
}
```

---

### 3.2 PortAudio HAL

```cpp
// hal/pc/audio_portaudio.hpp
#pragma once
#include <portaudio.h>
#include <atomic>
#include <mutex>
#include <vector>

class AudioPortAudio {
public:
    AudioPortAudio();
    ~AudioPortAudio();
    
    bool init();
    void close();
    
    void set_sample_rate(uint32_t rate);
    void set_volume(int8_t volume_db);
    
    void output_start();
    void output_stop();
    void input_start();
    void input_stop();
    
    // Called by baseband to push audio samples
    void write_samples(const int16_t* samples, size_t count);
    
    // Called by baseband to get mic samples
    size_t read_samples(int16_t* samples, size_t count);
    
private:
    static int output_callback(const void* input, void* output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData);
    
    static int input_callback(const void* input, void* output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData);
    
    PaStream* output_stream_ = nullptr;
    PaStream* input_stream_ = nullptr;
    
    uint32_t sample_rate_ = 24000;
    float volume_linear_ = 0.1f;
    
    // Ring buffers
    std::vector<int16_t> output_buffer_;
    std::atomic<size_t> output_read_pos_{0};
    std::atomic<size_t> output_write_pos_{0};
    
    std::vector<int16_t> input_buffer_;
    std::atomic<size_t> input_read_pos_{0};
    std::atomic<size_t> input_write_pos_{0};
    
    std::mutex mutex_;
};
```

---

### 3.3 Baseband Emulator

```cpp
// hal/pc/baseband_emulator.hpp
#pragma once
#include "baseband_processor.hpp"
#include "spi_image.hpp"
#include <thread>
#include <atomic>
#include <memory>

// Forward declare all processor classes
class AMAudioProcessor;
class NFMAudioProcessor;
class WFMAudioProcessor;
class AFSKRxProcessor;
class CaptureProcessor;
// ... etc

class BasebandEmulator {
public:
    BasebandEmulator();
    ~BasebandEmulator();
    
    void init(RFSoapySDR* rf, AudioPortAudio* audio);
    
    // Load processor by image tag (same as run_image)
    void load_processor(spi_flash::image_tag_t tag);
    
    // Start/stop processing
    void start();
    void stop();
    
    // Configuration passthrough
    void set_sample_rate(uint32_t rate);
    void on_message(const Message* msg);
    
    // Spectrum
    void spectrum_start();
    void spectrum_stop();
    
    // Capture/Replay
    void capture_start(CaptureConfig* config);
    void capture_stop();
    
private:
    void processing_thread();
    
    std::unique_ptr<BasebandProcessor> processor_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    
    RFSoapySDR* rf_ = nullptr;
    AudioPortAudio* audio_ = nullptr;
    
    uint32_t sample_rate_ = 2400000;
    bool spectrum_enabled_ = false;
};
```

**Implementation**:

```cpp
// hal/pc/baseband_emulator.cpp
#include "baseband_emulator.hpp"

// Include all processor implementations
#include "proc_am_audio.hpp"
#include "proc_nfm_audio.hpp"
#include "proc_wfm_audio.hpp"
#include "proc_afskrx.hpp"
#include "proc_capture.hpp"
#include "proc_replay.hpp"
// ... etc

void BasebandEmulator::load_processor(spi_flash::image_tag_t tag) {
    stop();
    
    // Factory pattern - create processor based on tag
    if (tag == spi_flash::image_tag_am_audio) {
        processor_ = std::make_unique<AMAudioProcessor>();
    }
    else if (tag == spi_flash::image_tag_nfm_audio) {
        processor_ = std::make_unique<NFMAudioProcessor>();
    }
    else if (tag == spi_flash::image_tag_wfm_audio) {
        processor_ = std::make_unique<WFMAudioProcessor>();
    }
    else if (tag == spi_flash::image_tag_afsk_rx) {
        processor_ = std::make_unique<AFSKRxProcessor>();
    }
    else if (tag == spi_flash::image_tag_capture) {
        processor_ = std::make_unique<CaptureProcessor>();
    }
    // ... handle all processor types
    else {
        std::cerr << "Unknown baseband image tag" << std::endl;
        return;
    }
    
    start();
}

void BasebandEmulator::start() {
    if (running_ || !processor_) return;
    
    running_ = true;
    thread_ = std::thread(&BasebandEmulator::processing_thread, this);
}

void BasebandEmulator::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
    processor_.reset();
}

void BasebandEmulator::processing_thread() {
    constexpr size_t BUFFER_SIZE = 2048;
    int8_t iq_buffer[BUFFER_SIZE * 2];
    
    while (running_) {
        // Read samples from RF
        int count = rf_->read_samples(iq_buffer, BUFFER_SIZE);
        
        if (count > 0) {
            // Create buffer struct matching Mayhem format
            buffer_c8_t buffer {
                iq_buffer,
                static_cast<size_t>(count),
                sample_rate_
            };
            
            // Process through current processor
            processor_->execute(buffer);
        }
        
        // Check for configuration messages
        Message msg;
        while (shared_memory.baseband_queue.pop(msg, 0)) {
            processor_->on_message(&msg);
        }
    }
}

void BasebandEmulator::on_message(const Message* msg) {
    if (processor_) {
        processor_->on_message(msg);
    }
}
```

---

## 4. Integration Example

### 4.1 Main Entry Point

```cpp
// main_pc.cpp
#include <SDL2/SDL.h>
#include "hal/pc/display_sdl.hpp"
#include "hal/pc/audio_portaudio.hpp"
#include "hal/pc/rf_soapysdr.hpp"
#include "hal/pc/baseband_emulator.hpp"
#include "event_m0.hpp"
#include "ui_navigation.hpp"

// Global HAL instances
DisplaySDL display_impl;
AudioPortAudio audio_hal;
RFSoapySDR rf_hal;
BasebandEmulator baseband_emu;

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Initialize HAL components
    if (!display_impl.init()) {
        std::cerr << "Display init failed" << std::endl;
        return 1;
    }
    
    if (!audio_hal.init()) {
        std::cerr << "Audio init failed" << std::endl;
        return 1;
    }
    
    // RF init is optional - can run without hardware
    if (!rf_hal.init()) {
        std::cerr << "Warning: No SDR hardware found, RF functions disabled" << std::endl;
    }
    
    // Initialize baseband emulator
    baseband_emu.init(&rf_hal, &audio_hal);
    
    // Create Mayhem UI - this is the original NavigationView
    ui::Context context;
    ui::NavigationView nav { context };
    
    // Start the event dispatcher (this is the main loop)
    EventDispatcher event_dispatcher;
    event_dispatcher.set_ui(&nav);
    event_dispatcher.run();
    
    // Cleanup
    baseband_emu.stop();
    rf_hal.close();
    audio_hal.close();
    SDL_Quit();
    
    return 0;
}
```

---

## 5. Testing Without Hardware

### 5.1 Mock RF Backend

```cpp
// hal/pc/rf_mock.hpp
// For testing without actual SDR hardware
class RFMock {
public:
    void load_iq_file(const std::string& path);
    void generate_test_signal(double freq_hz, double amplitude);
    
    // Same interface as RFSoapySDR
    int read_samples(int8_t* buffer, size_t count);
    
private:
    std::vector<int8_t> iq_data_;
    size_t position_ = 0;
    bool loop_ = true;
};
```

### 5.2 Compile-Time Backend Selection

```cmake
# Enable mock RF for testing
option(USE_MOCK_RF "Use mock RF backend for testing" OFF)

if(USE_MOCK_RF)
    add_definitions(-DUSE_MOCK_RF=1)
    set(RF_SOURCE hal/pc/rf_mock.cpp)
else()
    set(RF_SOURCE hal/pc/rf_soapysdr.cpp)
endif()
```

---

## 6. Summary

### Key Principles

1. **Same API, Different Implementation**: All shim files preserve exact original API signatures
2. **Compile-Time Switching**: `#ifdef PORTAPACK_PC_EMULATOR` selects implementation
3. **Zero App Changes**: Applications compile without modification
4. **Modular HAL**: Each hardware subsystem has its own interface/implementation pair
5. **Thread Safety**: PC implementation uses proper synchronization

### Files to Create

| Shim File | Wraps |
|-----------|-------|
| `shim/display_shim.cpp` | `portapack::display` |
| `shim/receiver_model_shim.cpp` | `receiver_model` |
| `shim/transmitter_model_shim.cpp` | `transmitter_model` |
| `shim/baseband_shim.cpp` | `baseband::*` functions |
| `shim/audio_shim.cpp` | `audio::*` functions |
| `shim/shared_memory_shim.cpp` | `shared_memory` |
| `shim/persistent_memory_shim.cpp` | `persistent_memory::*` |
| `shim/file_shim.cpp` | `File` class |
| `shim/event_shim.cpp` | `EventDispatcher` |

### HAL Implementations

| PC HAL | Replaces |
|--------|----------|
| `hal/pc/display_sdl.cpp` | ILI9341 LCD |
| `hal/pc/audio_portaudio.cpp` | WM8731/AK4951 codec |
| `hal/pc/rf_soapysdr.cpp` | MAX2837/RFFC5072 |
| `hal/pc/baseband_emulator.cpp` | M4 core execution |
| `hal/pc/input_sdl.cpp` | GPIO buttons/encoder |
| `hal/pc/storage_filesystem.cpp` | SD card via SDIO |

---

*Document Version: 2.0 - Shim Layer Architecture*
