#pragma once
#include <memory>
#include <atomic>
#include <iostream>

constexpr size_t MAX_UDP_PAYLOAD_SIZE = 1472;

// Single producer single consumer queue, to be used for the network thread (producer) writing 
// the extracted UDP payloads from the mmap'd shared ring buffer to the _buf below. Then the parser thread
// (consumer) reads from _buf and calls parseMessage() for each element in buf
template <typename T>
class SPSCQ {
    alignas(64) std::atomic<uint32_t> _write_idx{0};
    alignas(64) std::atomic<uint32_t> _read_idx{0};
    const size_t _capacity{0};
    const size_t _mask{0};
    std::unique_ptr<T []> _buf; // automatically cleans up buf after SPSCQ goes out of scope RAII

public:
    SPSCQ(size_t capacity): _capacity(capacity), _mask(capacity - 1) {
        if ((capacity & (capacity - 1)) != 0) throw std::runtime_error("Failed to construct SPSCQ: Capacity must be a power of 2");
        if (capacity < 2) throw std::runtime_error("Failed to create SPSCQ: Capcity must be at least 2");
        _buf = std::make_unique<T[]>(capacity);
    }

    // Delete copy/move constructors/assignment operators
    SPSCQ(const SPSCQ&) = delete;
    SPSCQ& operator=(const SPSCQ&) = delete;
    SPSCQ(const SPSCQ&&) = delete;
    SPSCQ& operator=(const SPSCQ&&) = delete;

    // Member functions

    [[nodiscard]] bool full() {
        uint32_t w = _write_idx.load(std::memory_order_relaxed);
        uint32_t r = _read_idx.load(std::memory_order_acquire);
        // If incrementing the current write index would overlap the read index
        // then we have reached queue capacity
        if (((w + 1) & _mask) == r) return true;
        return false;
    }

    [[nodiscard]] bool empty() {
        uint32_t w = _write_idx.load(std::memory_order_acquire);
        uint32_t r = _read_idx.load(std::memory_order_relaxed);
        // If the write and read indexes are equal, then the queue is empty
        if (w == r) return true;
        return false;
    }

    [[nodiscard]] size_t size() {
        // write index marks the tail of the queue, read marks the head
        // so write - read gives us the length, but must be bitwise AND with
        // mask to ensure result is within capacity
        uint32_t w = _write_idx.load(std::memory_order_acquire);
        uint32_t r = _read_idx.load(std::memory_order_acquire);
        return ((w - r) & _mask);
    }

    bool push(const T& item) {
        if (full()) return false; // If full, we cannot push
        // Otherwise, we can set the current write index of the buffer
        // to the input item, then increment the write index (ensuring within 
        // capacity bounds)
        uint32_t w = _write_idx.load(std::memory_order_relaxed);
        _buf[w] = item;
        _write_idx.store((w + 1) & _mask, std::memory_order_release);
        return true;
    }

    bool pop(T& item){
        if (empty()) return false; // If empty, we cannot pop
        // Otherwise we can safely read from the buffer into the input item
        // and increment the read index (ensuring within capacity bounds)
        uint32_t r = _read_idx.load(std::memory_order_relaxed);
        item = _buf[r];
        _read_idx.store((r + 1) & _mask, std::memory_order_release);
        return true;
    }
};

// Type of buffer holding UDP payloads, produced by network thread, consumed by parser thread
struct ParsingBuffer {
    char data[MAX_UDP_PAYLOAD_SIZE];
    size_t size;
};

// Class for managing the buffer pool, instantiated in main network thread, the freeBufferPool
// is public and can be popped/pushed by both the network and parsing threads
class BufferPool {
    std::unique_ptr<ParsingBuffer[]> _allPayloads;
    size_t _size;

public:
    BufferPool(size_t size): _allPayloads(std::make_unique<ParsingBuffer[]>(size)), _size(size) {
        for (int i = 0; i < size; i++) {
            freeBufferPool.push(&_allPayloads[i]);
        }
    }
    SPSCQ<ParsingBuffer*> freeBufferPool{_size};
};

// Parsing thread callable
void parserThread(std::shared_ptr<SPSCQ<ParsingBuffer*>>, const int &, BufferPool &);