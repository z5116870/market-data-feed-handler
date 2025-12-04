#include "gtest/gtest.h"
#include "sequencer.h"
#include <random>

// Helper: reset global state before each test
void resetGlobalState() {
    GlobalState::parsedMessages = 0;
    GlobalState::outOfOrderMessages = 0;
    GlobalState::lostMessages = 0;
    GlobalState::duplicates = 0;
    GlobalState::nextSeq = UINT32_MAX;
    GlobalState::highestSeq = 0;
    GlobalState::gapExists = false;
    GlobalState::gapTimeout = false;
    for (size_t i = 0; i < WINDOW_SIZE; ++i) GlobalState::seen[i] = 0;
}

// --- In-order test ---
TEST(Sequencer, InOrderMessages) {
    resetGlobalState();
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(2);
    checkAndSetGlobalState(3);

    EXPECT_EQ(GlobalState::parsedMessages.load(), 3);
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 0);
    EXPECT_EQ(GlobalState::duplicates.load(), 0);
    EXPECT_FALSE(GlobalState::gapExists.load());
}

// --- Out-of-order test ---
TEST(Sequencer, OutOfOrderMessages) {
    resetGlobalState();
    checkAndSetGlobalState(2);
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(3);

    EXPECT_EQ(GlobalState::parsedMessages.load(), 2); // Only seq 2 processed in-order
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 0); // seq 3 seen as out-of-order
    EXPECT_FALSE(GlobalState::gapExists.load());

    checkAndSetGlobalState(5);
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 1);
    EXPECT_TRUE(GlobalState::gapExists.load());
}

// --- Duplicate test ---
TEST(Sequencer, DuplicateMessages) {
    resetGlobalState();
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(1); // duplicate
    checkAndSetGlobalState(2);

    EXPECT_EQ(GlobalState::parsedMessages.load(), 2);
    EXPECT_EQ(GlobalState::duplicates.load(), 1);
}

// --- Gap timeout test ---
TEST(Sequencer, GapTimeoutHandling) {
    resetGlobalState();
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(3); // seq 2 missing -> gap opened

    EXPECT_TRUE(GlobalState::gapExists.load());
    GlobalState::highestSeq = 3;

    GlobalState::gapTimeout = true;
    handleGapTimeout();

    EXPECT_EQ(GlobalState::lostMessages.load(), 1); // seq 2 lost
    EXPECT_FALSE(GlobalState::gapExists.load());
    EXPECT_EQ(GlobalState::nextSeq.load(), 4); // highestSeq + 1
}

// --- Stress test ---
TEST(Sequencer, HighVolumeRandom) {
    resetGlobalState();
    const int N = 10000;
    std::vector<uint32_t> seqs;
    for (uint32_t i = 1; i <= N; ++i) seqs.push_back(i);
    std::shuffle(seqs.begin(), seqs.end(), std::mt19937{std::random_device{}()});

    for (auto s : seqs) checkAndSetGlobalState(s);

    // Handle any remaining gaps
    if (GlobalState::gapExists.load()) handleGapTimeout();

    // Draining should handle parsing all leftover messages (check line 67 in sequence.h)
    // So the sum of parsedMessages and duplicates should equal the original number of messages, N
    EXPECT_EQ(GlobalState::parsedMessages.load() + GlobalState::duplicates.load(), N);
}
