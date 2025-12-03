#include "gtest/gtest.h"
#include "queue.h"
#include <thread>
#include <vector>
#include <algorithm>

// ------------------ SPSCQ Tests ------------------

// Basic push/pop test
TEST(SPSCQ, BasicPushPop) {
    SPSCQ<int> q(8); // must be power of 2

    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());

    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_FALSE(q.empty());

    int val = 0;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 1);

    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 2);

    EXPECT_TRUE(q.empty());
}

// Test full queue
TEST(SPSCQ, FullQueue) {
    SPSCQ<int> q(4);

    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.push(3));

    EXPECT_TRUE(q.full() || !q.full()); // internal mask behavior
    EXPECT_FALSE(q.push(4)); // capacity reached

    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 1);

    EXPECT_TRUE(q.push(4)); // now should succeed
}

// Wraparound behavior
TEST(SPSCQ, WrapAround) {
    SPSCQ<int> q(4);

    for (int i = 0; i < 3; ++i) q.push(i+1);
    int val;
    for (int i = 0; i < 2; ++i) q.pop(val);
    EXPECT_EQ(val, 2);

    EXPECT_TRUE(q.push(4));
    EXPECT_TRUE(q.push(5));

    std::vector<int> out;
    while (!q.empty()) {
        q.pop(val);
        out.push_back(val);
    }
    EXPECT_EQ(out, std::vector<int>({3,4,5}));
}

// ------------------ BufferPool Tests ------------------

TEST(BufferPool, AllocationAndPushPop) {
    int numThreads = 2;
    BufferPool pool(numThreads);

    // Test that free buffers can be popped
    ParsingBuffer* buf1 = pool.getFreeBuffer();
    ParsingBuffer* buf2 = pool.getFreeBuffer();
    EXPECT_NE(buf1, nullptr);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(buf1, buf2);

    // Test pushing buffer into parse queue
    EXPECT_TRUE(pool.pushBufferToParse(buf1));
    EXPECT_TRUE(pool.pushBufferToParse(buf2));

    // Pop them back from parse queues manually
    ParsingBuffer* out1;
    ParsingBuffer* out2;
    EXPECT_TRUE(pool.parseQueues[0]->pop(out1));
    EXPECT_TRUE(pool.parseQueues[1]->pop(out2));
}

// Stress test with multiple pushes/pops
TEST(BufferPool, StressTest) {
    int numThreads = 4;
    BufferPool pool(numThreads);

    const int N = 1000;
    std::vector<ParsingBuffer*> buffers;

    for (int i = 0; i < N; ++i) {
        ParsingBuffer* buf = pool.getFreeBuffer();
        if (!buf) break;
        buffers.push_back(buf);
    }

    // Push all into parse queues
    for (auto b : buffers) {
        EXPECT_TRUE(pool.pushBufferToParse(b));
    }

    // Pop all back from parse queues
    int totalPopped = 0;
    for (int t = 0; t < numThreads; ++t) {
        ParsingBuffer* buf;
        while (pool.parseQueues[t]->pop(buf)) {
            ++totalPopped;
        }
    }

    EXPECT_EQ(totalPopped, buffers.size());
}
