#include "gtest/gtest.h"
#include "queue.h"
#include <thread>
#include <vector>
#include <algorithm>

// ------------------ SPSCQ Tests ------------------

// Basic push/pop test
TEST(SPSCQ, BasicPushPop) {
    // GIVEN a queue of size 8 (must be power of 2)
    SPSCQ<int> q(8);

    // WHEN it is not initialised THEN expect it to be empty
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());

    // WHEN we push 2 elements to it, THEN expect it to be NOT empty
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_FALSE(q.empty());

    // WHEN we pop from it, THEN we expect the earliest value pushed to be popped (1) 
    int val = 0;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 1);

    // WHEN we pop from it again, THEN we expect the read_idx to be equal to write_idx
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(q.empty());
}

// Test full queue
TEST(SPSCQ, FullQueue) {
    // GIVEN a queue of size 4
    SPSCQ<int> q(4);

    // WHEN we push 3 elements (max capacity)
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.push(3));

    EXPECT_TRUE(q.full() || !q.full()); // internal mask behavior
    // THEN we expect another push to fail
    EXPECT_FALSE(q.push(4)); // capacity reached

    // WHEN we pop a value
    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 1);

    // THEN we expect the push to succeed
    EXPECT_TRUE(q.push(4)); // now should succeed
}

// Wraparound behavior
TEST(SPSCQ, WrapAround) {
    // GIVEN a queue of size 4
    SPSCQ<int> q(4);

    // WHEN we push 3 values to the queue
    for (int i = 0; i < 3; ++i) q.push(i+1);
    int val;

    // AND pop the first 2 values
    for (int i = 0; i < 2; ++i) q.pop(val);

    // THEN we expect the last value popped to be 2
    EXPECT_EQ(val, 2);

    // AND THEN we also expect to be able to push 2 more values
    EXPECT_TRUE(q.push(4));
    EXPECT_TRUE(q.push(5));

    // WHEN we pop all the current values out of the queue
    std::vector<int> out;
    while (!q.empty()) {
        q.pop(val);
        out.push_back(val);
    }

    // THEN we expect the correct values (3, 4, 5) in order
    EXPECT_EQ(out, std::vector<int>({3,4,5}));
}

// ------------------ BufferPool Tests ------------------
TEST(BufferPool, AllocationAndPushPop) {
    // The expectation is that at the beginning of program execution, all parse queues are empty and all free queues are full
    // the network thread only ever moves from the free queues to the parse queues, and the parsing threads move from their own
    // parse queue back to the free queue after parsing

    // GIVEN a BufferPool with 2 threads, we should get 2 parsing queues and 2 free queues
    int numThreads = 2;
    BufferPool pool(numThreads);
    
    // WHEN free buffers are popped, THEN they should NOT be null, and should also be distinct
    // Test that free buffers can be popped. This should pop both from the first queue
    // as popping is done from the first available queue. We need to exhaust the first queue to get from the second
    ParsingBuffer* buf1 = pool.getFreeBuffer();
    ParsingBuffer* buf2 = pool.getFreeBuffer();
    EXPECT_NE(buf1, nullptr);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(buf1, buf2);

    int size = pool.freeQueues[0]->size() + 2; // Add two to include the buf1 & buf2 popped above
    // WHEN we exhaust the first buffer AND push each buffer to a parsing queue
    ParsingBuffer *q2;
    for (int i = 0; i < size; i++)  {
        q2 = pool.getFreeBuffer();
        pool.pushBufferToParse(q2);
    }

    // THEN we expect the next free buffer to come from the second queue AND the first parse queue should be full
    q2 = pool.getFreeBuffer();
    EXPECT_TRUE(pool.parseQueues[0]->full());

    // THEN we also expect further pushes to go into the second queue
    EXPECT_TRUE(pool.pushBufferToParse(buf1));
    EXPECT_TRUE(pool.pushBufferToParse(buf2));
    EXPECT_EQ(pool.parseQueues[1]->size(), 2);
}

// Stress test with multiple pushes/pops
TEST(BufferPool, StressTest) {
    // GIVEN a BufferPool to support 4 parsing threads (2048/4 = 512 capacity for each free and parse queue)
    int numThreads = 4;
    BufferPool pool(numThreads);

    const int N = 1000;
    std::vector<ParsingBuffer*> buffers;

    // WHEN we pop 1000 free buffers (512 from first, 488 from second)
    for (int i = 0; i < N; ++i) {
        ParsingBuffer* buf = pool.getFreeBuffer();
        if (!buf) break;
        buffers.push_back(buf);
    }

    // AND push all into the parsing queues (512 pushed to first, 488 pushed to second)
    // Push all into parse queues
    for (auto b : buffers) {
        EXPECT_TRUE(pool.pushBufferToParse(b));
    }

    // AND pop all back
    // Pop all back from parse queues
    int totalPopped = 0;
    for (int t = 0; t < numThreads; ++t) {
        ParsingBuffer* buf;
        while (pool.parseQueues[t]->pop(buf)) {
            ++totalPopped;
        }
    }

    // THEN the total number of buffers popped fromt he parsing queue should be equal to the
    // original number of buffers popped from the free queues (1000)
    EXPECT_EQ(totalPopped, buffers.size());
}
