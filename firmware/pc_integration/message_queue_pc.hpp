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

#ifndef __MESSAGE_QUEUE_PC_HPP__
#define __MESSAGE_QUEUE_PC_HPP__

#ifdef PORTAPACK_PC_EMULATOR

#include <cstdint>
#include <cstddef>
#include <array>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <cstring>

/**
 * @brief Message base class for PC emulator.
 *
 * Provides the same interface as the hardware Message class
 * but without ChibiOS dependencies.
 */
class Message {
public:
    static constexpr size_t MAX_SIZE = 512;

    enum class ID : uint32_t {
        RSSIStatistics = 0,
        BasebandStatistics = 1,
        ChannelStatistics = 2,
        DisplayFrameSync = 3,
        AudioStatistics = 4,
        Shutdown = 5,
        TPMSPacket = 6,
        ACARSPacket = 7,
        AISPacket = 8,
        ERTPacket = 9,
        SondePacket = 10,
        UpdateSpectrum = 11,
        NBFMConfigure = 12,
        WFMConfigure = 13,
        AMConfigure = 14,
        ChannelSpectrumConfig = 15,
        SpectrumStreamingConfig = 16,
        DisplaySleep = 17,
        CaptureConfig = 18,
        CaptureThreadDone = 19,
        ReplayConfig = 20,
        ReplayThreadDone = 21,
        AFSKRxConfigure = 22,
        StatusRefresh = 23,
        SampleRateConfig = 24,
        BTLERxConfigure = 25,
        NRFRxConfigure = 26,
        TXProgress = 27,
        Retune = 28,
        TonesConfigure = 29,
        AFSKTxConfigure = 30,
        PitchRSSIConfigure = 31,
        OOKConfigure = 32,
        RDSConfigure = 33,
        AudioTXConfig = 34,
        POCSAGConfigure = 35,
        DTMFTXConfig = 36,
        ADSBConfigure = 37,
        JammerConfigure = 38,
        WidebandSpectrumConfig = 39,
        FSKConfigure = 40,
        SSTVConfigure = 41,
        SigGenConfig = 42,
        SigGenTone = 43,
        POCSAGPacket = 44,
        ADSBFrame = 45,
        AFSKData = 46,
        TestAppPacket = 47,
        RequestSignal = 48,
        FIFOData = 49,
        AudioLevelReport = 50,
        CodedSquelch = 51,
        AudioSpectrum = 52,
        APRSPacket = 53,
        APRSRxConfigure = 54,
        SpectrumPainterBufferRequestConfigure = 55,
        SpectrumPainterBufferResponseConfigure = 56,
        POCSAGStats = 57,
        FSKRxConfigure = 58,
        BlePacket = 58,
        BTLETxConfigure = 59,
        SubGhzFPRxConfigure = 60,
        WeatherData = 61,
        SubGhzDData = 62,
        GPSPosData = 63,
        OrientationData = 64,
        EnvironmentData = 65,
        AudioBeep = 66,
        PocsagTosend = 67,
        BatteryStateData = 68,
        ProtoViewData = 69,
        FreqChangeCommand = 70,
        I2CDevListChanged = 71,
        LightData = 72,
        WeFaxRxConfigure = 73,
        WeFaxRxStatusData = 74,
        WeFaxRxImageData = 75,
        WFMAMConfigure = 76,
        NoaaAptRxConfigure = 77,
        NoaaAptRxStatusData = 78,
        NoaaAptRxImageData = 79,
        FSKPacket = 80,
        EPIRBPacket = 81,
        MAX
    };

    constexpr Message(ID id) : id{id} {}

    const ID id;
};

/**
 * @brief PC-compatible MessageQueue using std::mutex.
 *
 * This replaces the ChibiOS-based MessageQueue for PC emulation.
 */
class MessageQueue {
public:
    MessageQueue() = default;

    // Constructor matching hardware interface (buffer not used on PC)
    MessageQueue(uint8_t* const, size_t) {}

    MessageQueue(const MessageQueue&) = delete;
    MessageQueue(MessageQueue&&) = delete;

    /**
     * @brief Push a message onto the queue.
     */
    template <typename T>
    bool push(const T& message) {
        static_assert(sizeof(T) <= Message::MAX_SIZE, "Message too large");
        static_assert(std::is_base_of<Message, T>::value, "Type must derive from Message");

        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size() >= MAX_QUEUE_SIZE) {
            return false;
        }

        // Copy message to buffer
        MessageBuffer buf;
        buf.size = sizeof(T);
        std::memcpy(buf.data.data(), &message, sizeof(T));
        queue_.push(buf);
        cv_.notify_one();

        return true;
    }

    /**
     * @brief Push a message and wait until queue is empty.
     */
    template <typename T>
    bool push_and_wait(const T& message) {
        if (!push(message)) {
            return false;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return queue_.empty(); });
        return true;
    }

    /**
     * @brief Handle all pending messages with a handler function.
     */
    template <typename HandlerFn>
    void handle(HandlerFn handler) {
        std::lock_guard<std::mutex> lock(mutex_);

        while (!queue_.empty()) {
            const auto& buf = queue_.front();
            const Message* msg = reinterpret_cast<const Message*>(buf.data.data());
            handler(msg);
            queue_.pop();
        }
        cv_.notify_all();
    }

    /**
     * @brief Check if queue is empty.
     */
    bool is_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Clear all messages.
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    static constexpr size_t MAX_QUEUE_SIZE = 32;

    struct MessageBuffer {
        std::array<uint8_t, Message::MAX_SIZE> data{};
        size_t size = 0;
    };

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<MessageBuffer> queue_;
};

#endif // PORTAPACK_PC_EMULATOR

#endif // __MESSAGE_QUEUE_PC_HPP__
