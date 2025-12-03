#include "gtest/gtest.h"
#include "queue.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

// Number of parsing threads
constexpr int NUM_THREADS = 4;
// Number of buffers per thread to push
constexpr int BUFFERS_PER_THREAD = 10000;

// Simulate producer writing random data to buffers
void producer(BufferPool &pool, std::atomic<bool> &doneFlag) {
    int pushed = 0;
    while (pushed < NUM_THREADS * BUFFERS_PER_THREAD) {
        ParsingBuffer *buf = pool.getFreeBuffer();
        if (!buf) continue; // No free buffer, spin
        buf->size = 42;     // arbitrary payload size
        buf->data[0] = pushed % 256;
        if (pool.pushBufferToParse(buf)) {
            pushed++;
        }
    }
    doneFlag.store(true, std::memory_order_release);
}

// Simulate parser threads consuming buffers
void consumer(BufferPool &pool, std::atomic<int> &consumedCount, std::atomic<bool> &doneFlag, int threadIndex) {
    ParsingBuffer* buf;
    while (!doneFlag.load(std::memory_order_acquire) || !pool.parseQueues[threadIndex]->empty()) {
        if (pool.parseQueues[threadIndex]->pop(buf)) {
            // simulate parseMessage()
            buf->data[0] += 1;
            // push back to freeQueue
            pool.freeQueues[threadIndex]->push(buf);
            consumedCount.fetch_add(1, std::memory_order_relaxed);
        } else {
            std::this_thread::yield(); // avoid busy spin
        }
    }
}

// Multithreaded stress test
TEST(BufferPool, MultithreadedStress) {
    BufferPool pool(NUM_THREADS);

    std::atomic<bool> producerDone(false);
    std::atomic<int> totalConsumed(0);

    std::thread prod(producer, std::ref(pool), std::ref(producerDone));

    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_THREADS; ++i) {
        consumers.emplace_back(consumer, std::ref(pool), std::ref(totalConsumed), std::ref(producerDone), i);
    }

    prod.join();
    for (auto &t : consumers) t.join();

    EXPECT_EQ(totalConsumed.load(), NUM_THREADS * BUFFERS_PER_THREAD);
}
