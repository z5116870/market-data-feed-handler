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
    // GIVEN a new sequencer state
    resetGlobalState();

    // WHEN we observe 3 packets in order
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(2);
    checkAndSetGlobalState(3);

    // THEN we expect there to be no unordinary data, only 3 parsedMessages
    EXPECT_EQ(GlobalState::parsedMessages.load(), 3);
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 0);
    EXPECT_EQ(GlobalState::duplicates.load(), 0);
    EXPECT_FALSE(GlobalState::gapExists.load());
}

// --- Out-of-order test ---
TEST(Sequencer, OutOfOrderMessages) {
    // GIVEN a new sequencer state
    resetGlobalState();

    // WHEN we observe out of order packets (2, 1, 3)
    checkAndSetGlobalState(2);
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(3);

    // THEN we expect packets 2 and 3 to be parsed, 1 is NOT parsed
    EXPECT_EQ(GlobalState::parsedMessages.load(), 2); // Only seq 2 processed in-order
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 0); // seq 3 seen as out-of-order
    EXPECT_FALSE(GlobalState::gapExists.load());

    // WHEN we get sequence number 5 (nextSeq is currently 4)
    checkAndSetGlobalState(5);

    // THEN we expect it to be logged as an OOO message and the gap to be opened
    EXPECT_EQ(GlobalState::outOfOrderMessages.load(), 1);
    EXPECT_TRUE(GlobalState::gapExists.load());

    // AND WHEN we handle the gap
    GlobalState::gapTimeout = true;
    handleGapTimeout();

    // THEN we expect 4 to be marked lost AND nextSeq to be 6
    EXPECT_EQ(GlobalState::lostMessages.load(), 1);
    EXPECT_EQ(GlobalState::nextSeq.load(), 6);
}

// --- Duplicate test ---
TEST(Sequencer, DuplicateMessages) {
    // GIVEN a new sequencer state
    resetGlobalState();

    // WHEN we get 1, 1, 2
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(1); // duplicate
    checkAndSetGlobalState(2);

    // WE expect 1 to be parsed only once, and the second 1 to be marked as a duplicate
    EXPECT_EQ(GlobalState::parsedMessages.load(), 2);
    EXPECT_EQ(GlobalState::duplicates.load(), 1);
}

// --- Gap timeout test ---
TEST(Sequencer, GapTimeoutHandling) {
    // GIVEN a new sequencer state
    resetGlobalState();

    // WHEN we get an OOO packet (3)
    checkAndSetGlobalState(1);
    checkAndSetGlobalState(3); // seq 2 missing -> gap opened

    // THEN we expect the highestSeq and gapExists to be set correctly
    EXPECT_TRUE(GlobalState::gapExists.load());
    GlobalState::highestSeq = 3;

    // WHEN the timeout expires
    GlobalState::gapTimeout = true;
    handleGapTimeout();

    // THEN we expect 2 to be lost, and nextSeq to be set to 4
    EXPECT_EQ(GlobalState::lostMessages.load(), 1); // seq 2 lost
    EXPECT_FALSE(GlobalState::gapExists.load());
    EXPECT_EQ(GlobalState::nextSeq.load(), 4); // highestSeq + 1
}

// --- Stress test ---
TEST(Sequencer, HighVolumeRandom) {
    // GIVEN a new sequencer state
    resetGlobalState();

    // WHEN we create an input of 10000 randomly placed sequence numbers 
    const int N = 10000;
    std::vector<uint32_t> seqs;
    for (uint32_t i = 1; i <= N; ++i) seqs.push_back(i);
    std::shuffle(seqs.begin(), seqs.end(), std::mt19937{std::random_device{}()});
    for (auto s : seqs) checkAndSetGlobalState(s);

    // AND the timeout expires
    if (GlobalState::gapExists.load()) handleGapTimeout();

    // THEN the draining should handle parsing all leftover messages (check line 67 in sequence.h)
    // and the sum of parsedMessages and duplicates should equal the original number of messages, N
    EXPECT_EQ(GlobalState::parsedMessages.load() + GlobalState::duplicates.load(), N);
}
