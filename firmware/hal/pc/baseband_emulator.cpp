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

#include "baseband_emulator.hpp"
#include "../interface/hal_rf.hpp"
#include "../interface/hal_audio.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace baseband_emu {

// ============================================================================
// BasebandEmulator Implementation
// ============================================================================

BasebandEmulator::BasebandEmulator() = default;

BasebandEmulator::~BasebandEmulator() {
    shutdown();
}

bool BasebandEmulator::init(hal::IRF* rf, hal::IAudio* audio) {
    rf_ = rf;
    audio_ = audio;
    std::cout << "BasebandEmulator initialized" << std::endl;
    return true;
}

void BasebandEmulator::shutdown() {
    stop();
    rf_ = nullptr;
    audio_ = nullptr;
}

void BasebandEmulator::load_processor(const ImageTag& tag) {
    // Stop current processor
    stop();

    // Create new processor
    processor_ = create_processor(tag);
    current_tag_ = tag;

    if (processor_) {
        processor_->set_sample_rate(sample_rate_);

        // Start processing thread
        should_stop_ = false;
        running_ = true;
        thread_ = std::thread(&BasebandEmulator::processing_thread, this);

        std::cout << "Loaded processor: " << tag.c[0] << tag.c[1]
                  << tag.c[2] << tag.c[3] << std::endl;
    } else {
        std::cerr << "Unknown processor tag: " << tag.c[0] << tag.c[1]
                  << tag.c[2] << tag.c[3] << std::endl;
    }
}

void BasebandEmulator::stop() {
    should_stop_ = true;

    // Wake up the processing thread
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_cv_.notify_all();
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    running_ = false;
    processor_.reset();
    current_tag_ = IMAGE_TAG_NONE;
}

void BasebandEmulator::set_sample_rate(uint32_t rate) {
    sample_rate_ = rate;
    if (processor_) {
        processor_->set_sample_rate(rate);
    }
}

void BasebandEmulator::send_message(const BaseMessage* msg) {
    if (!msg) return;

    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.push(*msg);
    queue_cv_.notify_one();
}

void BasebandEmulator::spectrum_start() {
    spectrum_enabled_ = true;
}

void BasebandEmulator::spectrum_stop() {
    spectrum_enabled_ = false;
}

void BasebandEmulator::processing_thread() {
    constexpr size_t BUFFER_SIZE = 2048;
    std::vector<int8_t> iq_buffer(BUFFER_SIZE * 2);  // IQ pairs
    std::vector<int16_t> audio_buffer(1024);

    while (!should_stop_) {
        // Read samples from RF HAL
        int samples_read = 0;
        if (rf_ && rf_->is_streaming()) {
            samples_read = rf_->read_samples(iq_buffer.data(), BUFFER_SIZE);
        }

        if (samples_read > 0 && processor_) {
            // Process IQ samples
            processor_->execute(iq_buffer.data(), samples_read);

            // Get audio output
            size_t audio_samples = processor_->get_audio_samples(
                audio_buffer.data(), audio_buffer.size()
            );

            // Send to audio callback
            if (audio_samples > 0 && audio_callback_) {
                audio_callback_(audio_buffer.data(), audio_samples);
            }

            // Write to audio HAL
            if (audio_samples > 0 && audio_) {
                audio_->write_samples(audio_buffer.data(), audio_samples);
            }
        }

        // Process pending configuration messages
        process_pending_messages();

        // Handle spectrum if enabled
        if (spectrum_enabled_ && processor_) {
            auto* spectrum_proc = dynamic_cast<SpectrumProcessor*>(processor_.get());
            if (spectrum_proc) {
                ChannelSpectrum spectrum;
                spectrum_proc->get_spectrum(spectrum);
                if (spectrum_callback_) {
                    spectrum_callback_(spectrum);
                }
            }
        }

        // If no RF streaming, just process messages and sleep briefly
        if (!rf_ || !rf_->is_streaming()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void BasebandEmulator::process_pending_messages() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    while (!message_queue_.empty()) {
        BaseMessage msg = message_queue_.front();
        message_queue_.pop();

        if (processor_) {
            processor_->on_message(&msg);
        }
    }
}

std::unique_ptr<BasebandProcessor> BasebandEmulator::create_processor(const ImageTag& tag) {
    if (tag == IMAGE_TAG_AM_AUDIO) {
        return std::make_unique<AMAudioProcessor>();
    }
    if (tag == IMAGE_TAG_NFM_AUDIO) {
        return std::make_unique<NFMAudioProcessor>();
    }
    if (tag == IMAGE_TAG_WFM_AUDIO) {
        return std::make_unique<WFMAudioProcessor>();
    }
    if (tag == IMAGE_TAG_SPECTRUM) {
        return std::make_unique<SpectrumProcessor>();
    }
    if (tag == IMAGE_TAG_CAPTURE) {
        return std::make_unique<CaptureProcessor>();
    }
    if (tag == IMAGE_TAG_TEST) {
        return std::make_unique<TestProcessor>();
    }

    // Default to test processor for unknown tags
    return std::make_unique<TestProcessor>();
}

// ============================================================================
// AMAudioProcessor Implementation
// ============================================================================

void AMAudioProcessor::execute(const int8_t* buffer, size_t count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    // Simple AM envelope detection
    for (size_t i = 0; i < count; i += decimation_factor_) {
        float i_val = buffer[i * 2] / 127.0f;
        float q_val = buffer[i * 2 + 1] / 127.0f;

        // Envelope = sqrt(I^2 + Q^2)
        float envelope = std::sqrt(i_val * i_val + q_val * q_val);

        // Scale to audio range
        int16_t audio_sample = static_cast<int16_t>(envelope * 16384);
        audio_buffer_.push_back(audio_sample);
    }
}

void AMAudioProcessor::on_message(const BaseMessage* msg) {
    if (msg->id == MessageID::AMConfigure) {
        // Handle AM configuration
    }
}

size_t AMAudioProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t to_copy = std::min(max_count, audio_buffer_.size());
    if (to_copy > 0) {
        std::memcpy(buffer, audio_buffer_.data(), to_copy * sizeof(int16_t));
        audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + to_copy);
    }
    return to_copy;
}

// ============================================================================
// NFMAudioProcessor Implementation
// ============================================================================

void NFMAudioProcessor::execute(const int8_t* buffer, size_t count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    // Simple FM demodulation using phase difference
    for (size_t i = 0; i < count; i += decimation_factor_) {
        float i_val = buffer[i * 2] / 127.0f;
        float q_val = buffer[i * 2 + 1] / 127.0f;

        // Calculate phase
        float phase = std::atan2(q_val, i_val);

        // Phase difference (instantaneous frequency)
        float phase_diff = phase - last_phase_;

        // Wrap phase difference to [-pi, pi]
        while (phase_diff > M_PI) phase_diff -= 2 * M_PI;
        while (phase_diff < -M_PI) phase_diff += 2 * M_PI;

        last_phase_ = phase;

        // Scale to audio range
        int16_t audio_sample = static_cast<int16_t>(phase_diff * 10000);
        audio_buffer_.push_back(audio_sample);
    }
}

void NFMAudioProcessor::on_message(const BaseMessage* msg) {
    if (msg->id == MessageID::NBFMConfigure) {
        // Handle NBFM configuration
        // squelch_level_ = msg->data[...];
    }
}

size_t NFMAudioProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t to_copy = std::min(max_count, audio_buffer_.size());
    if (to_copy > 0) {
        std::memcpy(buffer, audio_buffer_.data(), to_copy * sizeof(int16_t));
        audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + to_copy);
    }
    return to_copy;
}

// ============================================================================
// WFMAudioProcessor Implementation
// ============================================================================

void WFMAudioProcessor::execute(const int8_t* buffer, size_t count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    // WFM demodulation similar to NFM but with different deviation
    for (size_t i = 0; i < count; i += decimation_factor_) {
        float i_val = buffer[i * 2] / 127.0f;
        float q_val = buffer[i * 2 + 1] / 127.0f;

        float phase = std::atan2(q_val, i_val);
        float phase_diff = phase - last_phase_;

        while (phase_diff > M_PI) phase_diff -= 2 * M_PI;
        while (phase_diff < -M_PI) phase_diff += 2 * M_PI;

        last_phase_ = phase;

        // WFM has wider deviation, adjust scaling
        int16_t audio_sample = static_cast<int16_t>(phase_diff * 5000);
        audio_buffer_.push_back(audio_sample);
    }
}

void WFMAudioProcessor::on_message(const BaseMessage* msg) {
    if (msg->id == MessageID::WFMConfigure) {
        // Handle WFM configuration
    }
}

size_t WFMAudioProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t to_copy = std::min(max_count, audio_buffer_.size());
    if (to_copy > 0) {
        std::memcpy(buffer, audio_buffer_.data(), to_copy * sizeof(int16_t));
        audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + to_copy);
    }
    return to_copy;
}

// ============================================================================
// SpectrumProcessor Implementation
// ============================================================================

SpectrumProcessor::SpectrumProcessor() {
    fft_window_.resize(fft_size_);
    fft_buffer_.resize(fft_size_ * 2);  // Complex
    spectrum_db_.resize(fft_size_, 0);

    // Create Hann window
    for (size_t i = 0; i < fft_size_; ++i) {
        fft_window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fft_size_ - 1)));
    }
}

void SpectrumProcessor::execute(const int8_t* buffer, size_t count) {
    std::lock_guard<std::mutex> lock(spectrum_mutex_);

    // Collect samples for FFT
    for (size_t i = 0; i < count && samples_collected_ < fft_size_; ++i) {
        fft_buffer_[samples_collected_ * 2] = buffer[i * 2] / 127.0f * fft_window_[samples_collected_];
        fft_buffer_[samples_collected_ * 2 + 1] = buffer[i * 2 + 1] / 127.0f * fft_window_[samples_collected_];
        samples_collected_++;
    }

    // When we have enough samples, compute power spectrum
    if (samples_collected_ >= fft_size_) {
        // Simple power spectrum (no actual FFT, just power calculation for demo)
        // In a real implementation, you'd use FFTW or similar
        for (size_t i = 0; i < fft_size_; ++i) {
            float power = fft_buffer_[i * 2] * fft_buffer_[i * 2] +
                         fft_buffer_[i * 2 + 1] * fft_buffer_[i * 2 + 1];
            float db = 10.0f * std::log10(power + 1e-10f);
            // Map -120 to 0 dB to 0-255
            int db_scaled = static_cast<int>((db + 120) * 255 / 120);
            spectrum_db_[i] = static_cast<uint8_t>(std::clamp(db_scaled, 0, 255));
        }
        samples_collected_ = 0;
    }
}

void SpectrumProcessor::on_message(const BaseMessage* msg) {
    if (msg->id == MessageID::SpectrumStreamingConfig) {
        // Handle spectrum configuration
    }
}

size_t SpectrumProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    (void)buffer;
    (void)max_count;
    return 0;  // Spectrum processor doesn't produce audio
}

void SpectrumProcessor::get_spectrum(ChannelSpectrum& spectrum) {
    std::lock_guard<std::mutex> lock(spectrum_mutex_);

    std::memcpy(spectrum.db, spectrum_db_.data(),
                std::min(size_t(256), spectrum_db_.size()));
    spectrum.sampling_rate = sample_rate_;
    spectrum.channel_filter_low_frequency = 0;
    spectrum.channel_filter_high_frequency = sample_rate_ / 2;
    spectrum.channel_filter_transition = 0;
}

// ============================================================================
// CaptureProcessor Implementation
// ============================================================================

void CaptureProcessor::execute(const int8_t* buffer, size_t count) {
    if (capture_callback_) {
        capture_callback_(buffer, count * 2);  // IQ pairs
    }
}

void CaptureProcessor::on_message(const BaseMessage* msg) {
    if (msg->id == MessageID::CaptureConfig) {
        // Handle capture configuration
    }
}

size_t CaptureProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    (void)buffer;
    (void)max_count;
    return 0;  // Capture processor doesn't produce audio
}

// ============================================================================
// TestProcessor Implementation
// ============================================================================

void TestProcessor::execute(const int8_t* buffer, size_t count) {
    (void)buffer;

    std::lock_guard<std::mutex> lock(audio_mutex_);

    // Generate a test tone
    float phase_inc = 2.0f * M_PI * tone_freq_ / 24000.0f;  // Assuming 24kHz audio

    size_t audio_samples = count / 100;  // Decimation
    for (size_t i = 0; i < audio_samples; ++i) {
        int16_t sample = static_cast<int16_t>(std::sin(phase_) * 8192);
        audio_buffer_.push_back(sample);
        phase_ += phase_inc;
        if (phase_ >= 2.0f * M_PI) phase_ -= 2.0f * M_PI;
    }
}

void TestProcessor::on_message(const BaseMessage* msg) {
    (void)msg;
    // Test processor ignores messages
}

size_t TestProcessor::get_audio_samples(int16_t* buffer, size_t max_count) {
    std::lock_guard<std::mutex> lock(audio_mutex_);

    size_t to_copy = std::min(max_count, audio_buffer_.size());
    if (to_copy > 0) {
        std::memcpy(buffer, audio_buffer_.data(), to_copy * sizeof(int16_t));
        audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + to_copy);
    }
    return to_copy;
}

} // namespace baseband_emu
