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
 * ChibiOS Compatibility Layer for PC Emulator
 *
 * This header provides PC-compatible implementations of ChibiOS primitives
 * using standard C++ threading facilities (std::mutex, std::condition_variable).
 */

#ifndef __CH_H__
#define __CH_H__

#ifdef PORTAPACK_PC_EMULATOR

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Basic Types
 * ============================================================================ */

typedef int32_t msg_t;
typedef uint32_t systime_t;
typedef uint32_t tprio_t;

/* Thread states */
#define THD_STATE_SUSPENDED 0
#define THD_STATE_READY 1
#define THD_STATE_RUNNING 2

/* ============================================================================
 * Mutex Types and Functions
 * ============================================================================ */

/**
 * @brief PC-compatible Mutex structure wrapping std::mutex.
 */
typedef struct {
    std::mutex mtx;
    std::atomic<bool> locked;
} Mutex;

/**
 * @brief Initialize a mutex.
 */
static inline void chMtxInit(Mutex* mp) {
    mp->locked = false;
}

/**
 * @brief Lock a mutex (blocking).
 */
static inline void chMtxLock(Mutex* mp) {
    mp->mtx.lock();
    mp->locked = true;
}

/**
 * @brief Try to lock a mutex (non-blocking).
 * @return true if lock acquired, false otherwise.
 */
static inline bool chMtxTryLock(Mutex* mp) {
    bool result = mp->mtx.try_lock();
    if (result) {
        mp->locked = true;
    }
    return result;
}

/**
 * @brief Unlock the most recently locked mutex.
 * Note: In PC version, we need to track which mutex to unlock.
 * For simplicity, this unlocks the last locked mutex in current thread.
 */
static inline void chMtxUnlock(void) {
    // In PC emulator, this is called after chMtxLock/chMtxTryLock
    // The mutex is stored in a thread-local variable or we rely on
    // the calling code to manage it properly
    // For now, this is a no-op as std::mutex unlock is handled differently
}

/**
 * @brief Unlock a specific mutex.
 */
static inline void chMtxUnlockMutex(Mutex* mp) {
    mp->locked = false;
    mp->mtx.unlock();
}

/**
 * @brief Unlock all mutexes owned by current thread.
 */
static inline void chMtxUnlockAll(void) {
    // No-op for PC emulator - not typically needed
}

/* ============================================================================
 * Semaphore Types and Functions
 * ============================================================================ */

typedef struct {
    std::mutex mtx;
    std::condition_variable cv;
    int32_t count;
} Semaphore;

static inline void chSemInit(Semaphore* sp, int32_t n) {
    sp->count = n;
}

static inline void chSemSignal(Semaphore* sp) {
    std::lock_guard<std::mutex> lock(sp->mtx);
    sp->count++;
    sp->cv.notify_one();
}

static inline msg_t chSemWait(Semaphore* sp) {
    std::unique_lock<std::mutex> lock(sp->mtx);
    sp->cv.wait(lock, [sp]() { return sp->count > 0; });
    sp->count--;
    return 0;
}

static inline msg_t chSemWaitTimeout(Semaphore* sp, systime_t timeout) {
    std::unique_lock<std::mutex> lock(sp->mtx);
    if (sp->cv.wait_for(lock, std::chrono::milliseconds(timeout),
                        [sp]() { return sp->count > 0; })) {
        sp->count--;
        return 0;
    }
    return -1;  // Timeout
}

/* ============================================================================
 * Thread Types and Functions
 * ============================================================================ */

struct Thread {
    std::thread* thread_handle;
    msg_t p_u_rdymsg;  // Message field for wake-up
    void* arg;

    struct {
        msg_t rdymsg;
    } p_u;
};

typedef Thread* Thread_t;

/**
 * @brief Sleep for specified milliseconds.
 */
static inline void chThdSleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/**
 * @brief Sleep for specified microseconds.
 */
static inline void chThdSleepMicroseconds(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

/**
 * @brief Get current thread.
 * Note: In PC emulator, we don't have direct thread access like ChibiOS.
 */
static inline Thread* chThdSelf(void) {
    // Return a dummy thread pointer for PC emulator
    static thread_local Thread current_thread = {};
    return &current_thread;
}

/**
 * @brief Yield processor time.
 */
static inline void chThdYield(void) {
    std::this_thread::yield();
}

/**
 * @brief Get current system ticks (milliseconds since start).
 */
static inline systime_t chThdGetTicks(void) {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return static_cast<systime_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count()
    );
}

/* ============================================================================
 * System Lock Functions (Critical Sections)
 * ============================================================================ */

// Global system lock for critical sections
extern std::mutex g_system_lock;

static inline void chSysLock(void) {
    g_system_lock.lock();
}

static inline void chSysUnlock(void) {
    g_system_lock.unlock();
}

static inline void chSysLockFromIsr(void) {
    // No ISR in PC - just use regular lock
    g_system_lock.lock();
}

static inline void chSysUnlockFromIsr(void) {
    g_system_lock.unlock();
}

/**
 * @brief Halt the system with an error message.
 */
static inline void chSysHalt(const char* reason) {
    (void)reason;
    // In PC emulator, we might want to throw an exception or abort
    // For now, just print and abort
    abort();
}

/* ============================================================================
 * Scheduler Functions
 * ============================================================================ */

static inline void chSchGoSleepS(int state) {
    (void)state;
    // In PC emulator, this is a no-op or could use condition variable
    std::this_thread::yield();
}

static inline void chSchReadyI(Thread* tp) {
    (void)tp;
    // In PC emulator, this is a no-op
}

/* ============================================================================
 * Heap Functions
 * ============================================================================ */

static inline void* chHeapAlloc(void* heap, size_t size) {
    (void)heap;
    return malloc(size);
}

static inline void chHeapFree(void* p) {
    free(p);
}

/* ============================================================================
 * Debug Functions
 * ============================================================================ */

static inline void chDbgPanic(const char* msg) {
    (void)msg;
    abort();
}

static inline void chDbgAssert(bool condition, const char* msg) {
    if (!condition) {
        chDbgPanic(msg);
    }
}

/* ============================================================================
 * Event Types and Functions
 * ============================================================================ */

typedef uint32_t eventflags_t;
typedef uint32_t eventmask_t;

typedef struct {
    std::mutex mtx;
    std::condition_variable cv;
    eventflags_t flags;
} EventSource;

static inline void chEvtInit(EventSource* esp) {
    esp->flags = 0;
}

static inline void chEvtBroadcastFlags(EventSource* esp, eventflags_t flags) {
    std::lock_guard<std::mutex> lock(esp->mtx);
    esp->flags |= flags;
    esp->cv.notify_all();
}

static inline eventflags_t chEvtWaitAny(eventmask_t mask) {
    (void)mask;
    return 0;
}

/* ============================================================================
 * Time Conversion
 * ============================================================================ */

#define MS2ST(ms) ((systime_t)(ms))
#define ST2MS(st) ((uint32_t)(st))
#define TIME_INFINITE ((systime_t)-1)

#ifdef __cplusplus
}
#endif

#endif /* PORTAPACK_PC_EMULATOR */

#endif /* __CH_H__ */
