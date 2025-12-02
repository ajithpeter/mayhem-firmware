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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __BASEBAND_EMULATOR_HPP__
#define __BASEBAND_EMULATOR_HPP__

#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

// Forward declarations for HAL
namespace hal {
    class IRF;
    class IAudio;
}

namespace baseband_emu {

/**
 * @brief Message types for inter-thread communication.
 * Mirrors the Message::ID enum from message.hpp
 */
enum class MessageID : uint32_t {
    RSSIStatistics = 0,
    BasebandStatistics = 1,
    ChannelStatistics = 2,
    DisplayFrameSync = 3,
    AudioStatistics = 4,
    Shutdown = 5,
    NBFMConfigure = 12,
    WFMConfigure = 13,
    AMConfigure = 14,
    ChannelSpectrumConfig = 15,
    SpectrumStreamingConfig = 16,
    CaptureConfig = 18,
    ReplayConfig = 20,
    AFSKRxConfigure = 22,
    SampleRateConfig = 24,
    OOKConfigure = 32,
    POCSAGConfigure = 35,
    ADSBConfigure = 37,
    FSKConfigure = 40,
    MAX
};

/**
 * @brief Image tag type matching spi_flash::image_tag_t
 */
struct ImageTag {
    char c[4];

    bool operator==(const ImageTag& other) const {
        return c[0] == other.c[0] && c[1] == other.c[1] &&
               c[2] == other.c[2] && c[3] == other.c[3];
    }

    operator bool() const {
        return c[0] != 0 || c[1] != 0 || c[2] != 0 || c[3] != 0;
    }
};

// Common image tags
constexpr ImageTag IMAGE_TAG_NONE = {0, 0, 0, 0};
constexpr ImageTag IMAGE_TAG_AM_AUDIO = {'P', 'A', 'M', 'A'};
constexpr ImageTag IMAGE_TAG_NFM_AUDIO = {'P', 'N', 'F', 'M'};
constexpr ImageTag IMAGE_TAG_WFM_AUDIO = {'P', 'W', 'F', 'M'};
constexpr ImageTag IMAGE_TAG_CAPTURE = {'P', 'C', 'A', 'P'};
constexpr ImageTag IMAGE_TAG_REPLAY = {'P', 'R', 'E', 'P'};
constexpr ImageTag IMAGE_TAG_ADSB_RX = {'P', 'A', 'D', 'R'};
constexpr ImageTag IMAGE_TAG_AFSK_RX = {'P', 'A', 'F', 'R'};
constexpr ImageTag IMAGE_TAG_POCSAG = {'P', 'P', 'O', 'C'};
constexpr ImageTag IMAGE_TAG_AIS = {'P', 'A', 'I', 'S'};
constexpr ImageTag IMAGE_TAG_SPECTRUM = {'P', 'S', 'P', 'E'};
constexpr ImageTag IMAGE_TAG_OOK = {'P', 'O', 'O', 'K'};
constexpr ImageTag IMAGE_TAG_SONDE = {'P', 'S', 'O', 'N'};
constexpr ImageTag IMAGE_TAG_TEST = {'P', 'T', 'S', 'T'};

/**
 * @brief Generic message structure for baseband communication.
 */
struct BaseMessage {
    MessageID id;
    uint8_t data[508];  // Max message size - 4 bytes for id
};

/**
 * @brief Spectrum data structure.
 */
struct ChannelSpectrum {
    uint8_t db[256];
    uint32_t sampling_rate;
    int32_t channel_filter_low_frequency;
    int32_t channel_filter_high_frequency;
    int32_t channel_filter_transition;
};

/**
 * @brief Abstract base class for baseband processors.
 *
 * Each processor type (AM, NFM, WFM, etc.) inherits from this
 * and implements the execute() and on_message() methods.
 */
class BasebandProcessor {
public:
    virtual ~BasebandProcessor() = default;

    /**
     * @brief Process a buffer of IQ samples.
     * @param buffer Pointer to IQ sample pairs (int8_t, interleaved I,Q)
     * @param count Number of IQ sample pairs
     */
    virtual void execute(const int8_t* buffer, size_t count) = 0;

    /**
     * @brief Handle a configuration message.
     * @param msg Message to process
     */
    virtual void on_message(const BaseMessage* msg) = 0;

    /**
     * @brief Get audio samples produced by the processor.
     * @param buffer Buffer to store audio samples
     * @param max_count Maximum number of samples to retrieve
     * @return Number of samples returned
     */
    virtual size_t get_audio_samples(int16_t* buffer, size_t max_count) = 0;

    /**
     * @brief Set the sample rate for processing.
     * @param rate Sample rate in Hz
     */
    virtual void set_sample_rate(uint32_t rate) { sample_rate_ = rate; }

protected:
    uint32_t sample_rate_ = 2400000;
};

/**
 * @brief Baseband emulator that manages processor loading and execution.
 *
 * This class emulates the M4 core functionality of the PortaPack,
 * running DSP processors in a separate thread.
 */
class BasebandEmulator {
public:
    // Callback types
    using AudioCallback = std::function<void(const int16_t*, size_t)>;
    using SpectrumCallback = std::function<void(const ChannelSpectrum&)>;
    using MessageCallback = std::function<void(const BaseMessage*)>;

    BasebandEmulator();
    ~BasebandEmulator();

    /**
     * @brief Initialize the emulator.
     * @param rf RF HAL for receiving/transmitting samples
     * @param audio Audio HAL for audio output
     * @return true on success
     */
    bool init(hal::IRF* rf, hal::IAudio* audio);

    /**
     * @brief Shutdown the emulator.
     */
    void shutdown();

    /**
     * @brief Load a processor by image tag.
     * @param tag Processor image tag
     */
    void load_processor(const ImageTag& tag);

    /**
     * @brief Stop the current processor.
     */
    void stop();

    /**
     * @brief Check if a processor is running.
     * @return true if running
     */
    bool is_running() const { return running_; }

    /**
     * @brief Set the sample rate.
     * @param rate Sample rate in Hz
     */
    void set_sample_rate(uint32_t rate);

    /**
     * @brief Get the current sample rate.
     * @return Sample rate in Hz
     */
    uint32_t sample_rate() const { return sample_rate_; }

    /**
     * @brief Send a message to the processor.
     * @param msg Message to send
     */
    void send_message(const BaseMessage* msg);

    /**
     * @brief Start spectrum streaming.
     */
    void spectrum_start();

    /**
     * @brief Stop spectrum streaming.
     */
    void spectrum_stop();

    /**
     * @brief Set callback for audio output.
     * @param callback Function to call with audio samples
     */
    void set_audio_callback(AudioCallback callback) { audio_callback_ = callback; }

    /**
     * @brief Set callback for spectrum data.
     * @param callback Function to call with spectrum data
     */
    void set_spectrum_callback(SpectrumCallback callback) { spectrum_callback_ = callback; }

    /**
     * @brief Set callback for messages from processor.
     * @param callback Function to call with messages
     */
    void set_message_callback(MessageCallback callback) { message_callback_ = callback; }

private:
    void processing_thread();
    void process_pending_messages();
    std::unique_ptr<BasebandProcessor> create_processor(const ImageTag& tag);

    // HAL references
    hal::IRF* rf_ = nullptr;
    hal::IAudio* audio_ = nullptr;

    // Current processor
    std::unique_ptr<BasebandProcessor> processor_;
    ImageTag current_tag_ = IMAGE_TAG_NONE;

    // Threading
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_stop_{false};

    // Message queue
    std::queue<BaseMessage> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Configuration
    std::atomic<uint32_t> sample_rate_{2400000};
    std::atomic<bool> spectrum_enabled_{false};

    // Callbacks
    AudioCallback audio_callback_;
    SpectrumCallback spectrum_callback_;
    MessageCallback message_callback_;

    // Spectrum data
    ChannelSpectrum spectrum_;
    std::mutex spectrum_mutex_;
};

// ============================================================================
// Processor Implementations
// ============================================================================

/**
 * @brief AM audio demodulation processor.
 */
class AMAudioProcessor : public BasebandProcessor {
public:
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

private:
    std::vector<int16_t> audio_buffer_;
    std::mutex audio_mutex_;
    size_t decimation_factor_ = 100;
};

/**
 * @brief Narrowband FM audio demodulation processor.
 */
class NFMAudioProcessor : public BasebandProcessor {
public:
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

private:
    std::vector<int16_t> audio_buffer_;
    std::mutex audio_mutex_;
    float last_phase_ = 0.0f;
    size_t decimation_factor_ = 100;
    uint8_t squelch_level_ = 0;
};

/**
 * @brief Wideband FM audio demodulation processor.
 */
class WFMAudioProcessor : public BasebandProcessor {
public:
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

private:
    std::vector<int16_t> audio_buffer_;
    std::mutex audio_mutex_;
    float last_phase_ = 0.0f;
    size_t decimation_factor_ = 50;
};

/**
 * @brief Spectrum analysis processor.
 */
class SpectrumProcessor : public BasebandProcessor {
public:
    SpectrumProcessor();
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

    void get_spectrum(ChannelSpectrum& spectrum);

private:
    std::vector<float> fft_window_;
    std::vector<float> fft_buffer_;
    std::vector<uint8_t> spectrum_db_;
    std::mutex spectrum_mutex_;
    size_t fft_size_ = 256;
    size_t samples_collected_ = 0;
};

/**
 * @brief Capture processor (raw IQ recording).
 */
class CaptureProcessor : public BasebandProcessor {
public:
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

    void set_capture_callback(std::function<void(const int8_t*, size_t)> cb) {
        capture_callback_ = cb;
    }

private:
    std::function<void(const int8_t*, size_t)> capture_callback_;
};

/**
 * @brief Test processor that generates a test pattern.
 */
class TestProcessor : public BasebandProcessor {
public:
    void execute(const int8_t* buffer, size_t count) override;
    void on_message(const BaseMessage* msg) override;
    size_t get_audio_samples(int16_t* buffer, size_t max_count) override;

private:
    std::vector<int16_t> audio_buffer_;
    std::mutex audio_mutex_;
    float phase_ = 0.0f;
    float tone_freq_ = 1000.0f;
};

} // namespace baseband_emu

#endif // __BASEBAND_EMULATOR_HPP__
