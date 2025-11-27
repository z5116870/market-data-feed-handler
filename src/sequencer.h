// Check for out of order, duplicates and lost packets
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "parse.h"
#include "cpu.h"
constexpr auto GAP_TIMEOUT = std::chrono::milliseconds(5);

// Global state struct, used for tracking parsing program metrics (aligned to cache block size)
struct alignas(64) GlobalState {
    // Metrics
    inline static uint32_t parsedMessages = 0;
    inline static uint32_t outOfOrderMessages = 0;
    inline static uint32_t lostMessages = 0;
    inline static uint32_t duplicates = 0;

    // Sequencer
    inline static std::atomic<uint32_t> nextSeq = 0;
    inline static std::atomic<uint32_t> highestSeq = 0;
    inline static std::atomic<bool> gapExists = false;
    // ring buffer (indexed using modulus operator) for tracking 
    // most recently seen sequence numbers. Crucial for detecting duplicates, out of order packets
    // and lost packets. The window begins with 
    inline static std::atomic<uint8_t> seen[WINDOW_SIZE];

    // Timer
    inline static std::atomic<bool> gapTimeout = false; // flag set by timer thread, main thread reads this and flushes bitset
    inline static std::atomic<bool> timerIsRunning = false; // bool for determining if timer is running
};

inline void checkAndSetGlobalState(const uint32_t &seq) {
    // *** Refer to Sequencer state diagram for more information ***
    // Set highest sequence number
    uint32_t expected = 0;
    GlobalState::nextSeq.compare_exchange_strong(expected, seq);

    uint32_t old = GlobalState::highestSeq.load(std::memory_order_relaxed);
    while(seq > old && GlobalState::highestSeq.compare_exchange_weak(old, seq, std::memory_order_relaxed)){};

    // 1. seq < nextSeq (duplicate)
    if (seq < GlobalState::nextSeq.load(std::memory_order_acquire)) {
        // DUPLICATE
        GlobalState::duplicates++;
        return;
    }

    // 2. seq == nextSeq (in-order)
    if (seq == GlobalState::nextSeq.load(std::memory_order_acquire)) {
        // Set the sliding window bitset
        GlobalState::seen[seq % WINDOW_SIZE].store(1, std::memory_order_release);
        GlobalState::parsedMessages++;
        GlobalState::nextSeq.fetch_add(1, std::memory_order_release);
        
        // if we are in GAP_OPEN state
        if (GlobalState::gapExists) {
            // ADVANCE_DRAIN
            while(GlobalState::seen[GlobalState::nextSeq.load(std::memory_order_relaxed) % WINDOW_SIZE].load(std::memory_order_acquire)) {
                GlobalState::nextSeq.fetch_add(1, std::memory_order_release);
                GlobalState::parsedMessages++;
            }
            // Does the gap still exist?
            old = GlobalState::nextSeq.load(std::memory_order_acquire);
            uint32_t high = GlobalState::highestSeq.load(std::memory_order_acquire);
            if (old > high) GlobalState::gapExists.store(false, std::memory_order_release);
        }

        // otherwise NO_GAP state, normal processing
        return;
    }

    // 3. seq > nextSeq (out-of-order)
    if (seq > GlobalState::nextSeq.load(std::memory_order_acquire) ){
        // enter GAP_OPEN (can already be in this state, that just means more gaps, but the timer does not reset
        // it runs on a separate thread and begins only if there is no gap currently open)
        if (!GlobalState::gapExists.load(std::memory_order_acquire)) {
            GlobalState::gapExists.store(true, std::memory_order_release);
        } 
        GlobalState::outOfOrderMessages++;
        GlobalState::seen[seq % WINDOW_SIZE].store(1, std::memory_order_release);
    }
}

// Handle the gap timeout after it expires (entering GAP_TIMEOUT state)
inline void handleGapTimeout() {
    // If the flag is not set, just return
    if (!GlobalState::gapTimeout.load(std::memory_order_acquire)) return;

    // Otherwise, flush the bitset. Iterate over the bitset and for every 
    // 0 found in between the low (nextSeq) and the high (highestSeq) increment
    // the lostMessages counter
    for (uint32_t seq = GlobalState::nextSeq.load(std::memory_order_acquire); seq <= GlobalState::highestSeq.load(std::memory_order_acquire); ++seq) {
        if (!GlobalState::seen[seq % WINDOW_SIZE].load(std::memory_order_acquire)) GlobalState::lostMessages++;
    }
    
    // Reset the timer and gap states
    GlobalState::gapExists.store(false, std::memory_order_release);
    GlobalState::gapTimeout.store(false, std::memory_order_release);

    // Set the next expected seqeunce number to the highest + 1 (everything before
    // is now either parsed or lost)
    GlobalState::nextSeq.store(GlobalState::highestSeq.load(std::memory_order_acquire) + 1);
    std::cout << "[GAP TIMEOUT] Gap Closed!\n";
}

// Function run for the timer thread, sets gapTimerExpired flag in GlobalState
// once timer expires. Main thread, which runs handleGapTimeout() for every message parsed,
// checks this flag to flush the seen bitset.
inline void gapTimer() {
    // Pin this thread to an isolated CPU so it can spin wait to its hearts content
    pinToCpu(1);
    raisePriority();
    while (GlobalState::timerIsRunning.load(std::memory_order_acquire)) {
        // spin wait
        if (GlobalState::gapExists.load(std::memory_order_acquire)) {
            // Once the gap exists, start the timer
            std::this_thread::sleep_for(GAP_TIMEOUT);
            // Timer has expired, handle the gap
            handleGapTimeout();
        };
    }
}