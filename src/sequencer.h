// Check for out of order, duplicates and lost packets
#pragma once
#include <thread>
#include "parse.h"
constexpr auto GAP_TIMEOUT = std::chrono::milliseconds(5);

inline void handleGapTimeout() {
    // If the flag is not set, just return
    if (!GlobalState::gapTimeout.load(std::memory_order_relaxed)) return;

    // Otherwise, flush the bitset. Iterate over the bitset and for every 
    // 0 found in between the low (nextSeq) and the high (highestSeq) increment
    // the lostMessages counter
    for (uint32_t seq = GlobalState::nextSeq; seq <= GlobalState::highestSeq; ++seq) {
        if (!GlobalState::seen[seq % WINDOW_SIZE]) GlobalState::lostMessages++;
    }
    // Reset the timer and gap states
    GlobalState::gapExists = false;
    GlobalState::gapTimeout.store(false, std::memory_order_relaxed);

    // Set the next expected seqeunce number to the highest + 1 (everything before
    // is now either parsed or lost)
    GlobalState::nextSeq = GlobalState::highestSeq + 1;
    std::cout << "[GAP TIMEOUT] Gap Closed!\n";
}

inline void checkAndSetGlobalState(const uint32_t &seq) {
    // *** Refer to Sequencer state diagram for more information ***
    // Set highest sequence number
    if (GlobalState::nextSeq == 0) GlobalState::nextSeq = seq;
    if (seq > GlobalState::highestSeq) GlobalState::highestSeq = seq;

    // 1. seq < nextSeq (duplicate)
    if (seq < GlobalState::nextSeq) {
        // DUPLICATE
        GlobalState::duplicates++;
        return;
    }

    // 2. seq == nextSeq (in-order)
    if (seq == GlobalState::nextSeq) {
        // Set the sliding window bitset
        GlobalState::seen[seq % WINDOW_SIZE] = 1;
        GlobalState::parsedMessages++;
        GlobalState::nextSeq++;
        
        // if we are in GAP_OPEN state
        if (GlobalState::gapExists) {
            // ADVANCE_DRAIN
            while(GlobalState::seen[GlobalState::nextSeq % WINDOW_SIZE]) {
                GlobalState::nextSeq++;
                GlobalState::parsedMessages++;
            }
            // Does the gap still exist?
            if (GlobalState::nextSeq > GlobalState::highestSeq) GlobalState::gapExists = false;
        }

        // otherwise NO_GAP state, normal processing
        return;
    }

    // 3. seq > nextSeq (out-of-order)
    if (seq > GlobalState::nextSeq) {
        // enter GAP_OPEN (can already be in this state, that just means more gaps, but the timer does not reset
        // it runs on a separate thread and begins only if there is no gap currently open)
        if (!GlobalState::gapExists) {
            GlobalState::gapExists = true;
            GlobalState::gapStartTime = std::chrono::steady_clock::now();
        } 
        GlobalState::outOfOrderMessages++;
        GlobalState::seen[seq % WINDOW_SIZE] = 1;
    }
}

// Function run for the timer thread, sets gapTimerExpired flag in GlobalState
// once timer expires. Main thread, which runs handleGapTimeout() for every message parsed,
// checks this flag to flush the seen bitset.
inline void gapTimer() {
    while (GlobalState::timerIsRunning) {
        if (GlobalState::gapExists) {
            // If a gap exists, run the timer until it exceeds GAP_TIMEOUT
            // then set the flag in the GlobalState struct, so the main thread
            // can use it to flush the bitset
            auto now = std::chrono::steady_clock::now();
            // we use std::memory_order_relaxed because this is the only thread setting this, we
            // dont care about any order we are only worried about whether or not the flag is set.
            if (now - GlobalState::gapStartTime >= GAP_TIMEOUT) GlobalState::gapTimeout.store(true, std::memory_order_relaxed);
        }
        // Sleep this thread for a fraction of the GAP_TIMEOUT so that it does
        // not behave like a spinlock and consume too much of the current CPU
        // just waiting for the timer to expier
        std::this_thread::sleep_for(GAP_TIMEOUT/10);
    }
}