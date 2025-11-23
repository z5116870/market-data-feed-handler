#pragma once
#include <cstddef>
#include <iostream>
#include <atomic>
#include <bitset>
#include "helper.h"

enum LogLevel {
    VERBOSE = 1, RAW, OFF
};

// Size of the sliding window used for determining whether packets were received
// out of order, as duplicates or lost
constexpr size_t WINDOW_SIZE = 65536;

// Global state struct, used for tracking parsing program metrics (aligned to cache block size)
struct alignas(64) GlobalState {
    // Metrics
    inline static uint32_t parsedMessages = 0;
    inline static uint32_t outOfOrderMessages = 0;
    inline static uint32_t lostMessages = 0;
    inline static uint32_t duplicates = 0;

    // Sequencer
    inline static uint32_t nextSeq = 0;
    inline static uint32_t highestSeq = 0;
    inline static bool gapExists = false;
    // ring buffer (indexed using modulus operator) for tracking 
    // most recently seen sequence numbers. Crucial for detecting duplicates, out of order packets
    // and lost packets. The window begins with 
    inline static std::bitset<WINDOW_SIZE> seen;

    // Timer
    inline static std::atomic<bool> gapTimeout = false; // flag set by timer thread, main thread reads this and flushes bitset
    inline static std::atomic<bool> timerIsRunning = false; // bool for determining if timer is running
    inline static std::chrono::steady_clock::time_point gapStartTime; // time the gap was first seen, does not change until timer expires
};

enum MessageSize {
    Trade = 36,
    OrderExecuted = 23,
    OrderExecutedWithPrice = 28,
    SystemEvent = 12,
    OrderCancelled = 23
};

// Logger class to print parsed messages
class Logger {
public:
    Logger(LogLevel l = LogLevel::VERBOSE): m_logLevel(l) {}
    void setLogLevel(LogLevel l) { m_logLevel = l; }

    // delete move/copy constructor/assignment operators for safety
    Logger(const Logger &) = delete;
    Logger& operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger& operator=(Logger &&) = delete;

    //  Templated logging function that logs based on the level
    template <typename MessageType>
    void log(const MessageType &m) const {
        switch (m_logLevel) {
            case LogLevel::VERBOSE:
                verbose(m);
                break;
            case LogLevel::RAW:
                raw(m);
                break;
            default:
                break;
        }
    }

private:
    LogLevel m_logLevel;

    // Nested templated logging function that logs based on input MessageType
    template <typename MessageType>
    void verbose(const MessageType &parsedMsg) const { std::cout << parsedMsg; }

    template <typename MessageType>
    void raw(const MessageType &parsedMsg) const { parsedMsg.getRawLog(); }
};


// Parsing structs (Updated to feature CRTP for static polymorphism, avoiding virtual function overhead)
template<typename DerivedMessage>
struct Message {
    char        messageType;
    uint64_t    timestamp;
    uint32_t    sequenceNumber;
    void        getRawLog() const {
        static_cast<const DerivedMessage*>(this)->getRawLogImpl();
    }
};

template<typename DerivedMessage>
struct OrderMessage: Message<DerivedMessage> {
    uint64_t    orderRefNumber;
};

struct TradeMessage: OrderMessage<TradeMessage> {
    char        buySellIndicator;
    uint32_t    shares;
    uint32_t    price;
    char        stock[8];
    void getRawLogImpl() const;
};

struct OrderExecutedMessage: OrderMessage<OrderExecutedMessage> {
    uint32_t    executedShares;
    void getRawLogImpl() const;
};

struct OrderExecutedWithPriceMessage: OrderMessage<OrderExecutedWithPriceMessage> {
    char        printable;
    uint32_t    executedPrice;
    uint32_t    executedShares;
    void getRawLogImpl() const;
};

struct SystemEventMessage: Message<SystemEventMessage> {
    char        eventCode;
    void getRawLogImpl() const;
};

struct OrderCancelMessage: OrderMessage<OrderCancelMessage> {
    uint32_t    cancelledShares;
    void getRawLogImpl() const;
};

// Parsing functions
void parseMessage(const char* buf, const ssize_t &len); 
ssize_t parseTrade(const char* buf, TradeMessage &t);
ssize_t parseOrderExecuted(const char* buf, OrderExecutedMessage &t);
ssize_t parseOrderWithPrice(const char* buf, OrderExecutedWithPriceMessage &t);
ssize_t parseSystemEvent(const char* buf, SystemEventMessage &t);
ssize_t parseOrderCancelled(const char* buf, OrderCancelMessage &t);

// Static parsing structs (fixed memory address means they will be cache hot, faster writes)
static TradeMessage tradeMsg{};
static OrderExecutedMessage orderExecutedMsg{};
static OrderExecutedWithPriceMessage orderExecutedWithPriceMsg{};
static SystemEventMessage sysMsg{};
static OrderCancelMessage orderCancelMsg{};

// Operator overloads for logging
std::ostream &operator<<(std::ostream &s, TradeMessage &t);
std::ostream &operator<<(std::ostream &s, OrderExecutedMessage &t);
std::ostream &operator<<(std::ostream &s, OrderExecutedWithPriceMessage &t);
std::ostream &operator<<(std::ostream &s, SystemEventMessage &t);
std::ostream &operator<<(std::ostream &s, OrderCancelMessage &t);