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

#include "audio_portaudio.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace hal {

AudioPortAudio::AudioPortAudio() {
    output_buffer_.resize(RING_BUFFER_SIZE, 0);
    input_buffer_.resize(RING_BUFFER_SIZE, 0);
}

AudioPortAudio::~AudioPortAudio() {
    shutdown();
}

bool AudioPortAudio::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "AudioPortAudio initialized, sample rate: " << sample_rate_ << " Hz" << std::endl;
    return true;
}

void AudioPortAudio::shutdown() {
    output_stop();
    input_stop();

    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        Pa_Terminate();
        initialized_ = false;
    }
}

void AudioPortAudio::set_sample_rate(uint32_t rate) {
    bool was_output_running = output_running_;
    bool was_input_running = input_running_;

    if (was_output_running) output_stop();
    if (was_input_running) input_stop();

    sample_rate_ = rate;

    if (was_output_running) output_start();
    if (was_input_running) input_start();
}

uint32_t AudioPortAudio::sample_rate() const {
    return sample_rate_;
}

void AudioPortAudio::set_volume(int8_t volume_db) {
    volume_db_ = std::max(int8_t(-60), std::min(int8_t(0), volume_db));
    volume_linear_ = db_to_linear(volume_db_);
}

int8_t AudioPortAudio::volume() const {
    return volume_db_;
}

void AudioPortAudio::output_start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || output_running_) {
        return;
    }

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        std::cerr << "No default output device" << std::endl;
        return;
    }

    outputParams.channelCount = 1;  // Mono
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &output_stream_,
        nullptr,
        &outputParams,
        sample_rate_,
        BUFFER_FRAMES,
        paClipOff,
        output_callback_static,
        this
    );

    if (err != paNoError) {
        std::cerr << "Failed to open output stream: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(output_stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start output stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(output_stream_);
        output_stream_ = nullptr;
        return;
    }

    output_running_ = true;
    std::cout << "Audio output started" << std::endl;
}

void AudioPortAudio::output_stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (output_stream_) {
        Pa_StopStream(output_stream_);
        Pa_CloseStream(output_stream_);
        output_stream_ = nullptr;
    }

    output_running_ = false;
}

void AudioPortAudio::input_start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || input_running_) {
        return;
    }

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        std::cerr << "No default input device" << std::endl;
        return;
    }

    inputParams.channelCount = 1;  // Mono
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &input_stream_,
        &inputParams,
        nullptr,
        sample_rate_,
        BUFFER_FRAMES,
        paClipOff,
        input_callback_static,
        this
    );

    if (err != paNoError) {
        std::cerr << "Failed to open input stream: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(input_stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start input stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(input_stream_);
        input_stream_ = nullptr;
        return;
    }

    input_running_ = true;
    std::cout << "Audio input started" << std::endl;
}

void AudioPortAudio::input_stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (input_stream_) {
        Pa_StopStream(input_stream_);
        Pa_CloseStream(input_stream_);
        input_stream_ = nullptr;
    }

    input_running_ = false;
}

size_t AudioPortAudio::write_samples(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return 0;

    size_t read_pos = output_read_pos_.load();
    size_t write_pos = output_write_pos_.load();
    size_t free_space = ring_buffer_free(read_pos, write_pos, output_buffer_.size());
    size_t to_write = std::min(count, free_space);

    for (size_t i = 0; i < to_write; ++i) {
        output_buffer_[write_pos] = samples[i];
        write_pos = (write_pos + 1) % output_buffer_.size();
    }

    output_write_pos_.store(write_pos);
    return to_write;
}

size_t AudioPortAudio::read_samples(int16_t* samples, size_t count) {
    if (!samples || count == 0) return 0;

    size_t read_pos = input_read_pos_.load();
    size_t write_pos = input_write_pos_.load();
    size_t available = ring_buffer_available(read_pos, write_pos, input_buffer_.size());
    size_t to_read = std::min(count, available);

    for (size_t i = 0; i < to_read; ++i) {
        samples[i] = input_buffer_[read_pos];
        read_pos = (read_pos + 1) % input_buffer_.size();
    }

    input_read_pos_.store(read_pos);
    return to_read;
}

void AudioPortAudio::set_output_callback(AudioCallback callback) {
    output_callback_ = callback;
}

void AudioPortAudio::set_input_callback(AudioCallback callback) {
    input_callback_ = callback;
}

bool AudioPortAudio::is_output_running() const {
    return output_running_;
}

bool AudioPortAudio::is_input_running() const {
    return input_running_;
}

int AudioPortAudio::output_callback_static(
    const void* input,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
) {
    (void)input;
    (void)timeInfo;
    (void)statusFlags;

    auto* self = static_cast<AudioPortAudio*>(userData);
    return self->output_callback(output, frameCount);
}

int AudioPortAudio::input_callback_static(
    const void* input,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
) {
    (void)output;
    (void)timeInfo;
    (void)statusFlags;

    auto* self = static_cast<AudioPortAudio*>(userData);
    return self->input_callback(input, frameCount);
}

int AudioPortAudio::output_callback(void* output, unsigned long frameCount) {
    int16_t* out = static_cast<int16_t*>(output);

    // If we have a user callback, use it
    if (output_callback_) {
        output_callback_(out, frameCount);
        // Apply volume
        for (unsigned long i = 0; i < frameCount; ++i) {
            out[i] = static_cast<int16_t>(out[i] * volume_linear_);
        }
        return paContinue;
    }

    // Otherwise, read from ring buffer
    size_t read_pos = output_read_pos_.load();
    size_t write_pos = output_write_pos_.load();
    size_t available = ring_buffer_available(read_pos, write_pos, output_buffer_.size());

    for (unsigned long i = 0; i < frameCount; ++i) {
        if (i < available) {
            out[i] = static_cast<int16_t>(output_buffer_[read_pos] * volume_linear_);
            read_pos = (read_pos + 1) % output_buffer_.size();
        } else {
            out[i] = 0;  // Silence if buffer underrun
        }
    }

    output_read_pos_.store(read_pos);
    return paContinue;
}

int AudioPortAudio::input_callback(const void* input, unsigned long frameCount) {
    const int16_t* in = static_cast<const int16_t*>(input);

    // If we have a user callback, use it
    if (input_callback_) {
        // Need to cast away const for the callback (it's expecting a buffer it can write to)
        // In this case, we're providing input data
        std::vector<int16_t> temp(in, in + frameCount);
        input_callback_(temp.data(), frameCount);
        return paContinue;
    }

    // Otherwise, write to ring buffer
    size_t read_pos = input_read_pos_.load();
    size_t write_pos = input_write_pos_.load();
    size_t free_space = ring_buffer_free(read_pos, write_pos, input_buffer_.size());

    size_t to_write = std::min(static_cast<size_t>(frameCount), free_space);
    for (size_t i = 0; i < to_write; ++i) {
        input_buffer_[write_pos] = in[i];
        write_pos = (write_pos + 1) % input_buffer_.size();
    }

    input_write_pos_.store(write_pos);
    return paContinue;
}

} // namespace hal
