#pragma once
#include <cstddef>
#include <iostream>
#include "helper.h"

enum LogLevel {
    VERBOSE = 1, RAW
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
void parseMessage(const char* buf, size_t len); 
void parseTrade(const char* buf, TradeMessage &t);
void parseOrderExecuted(const char* buf, OrderExecutedMessage &t);
void parseOrderWithPrice(const char* buf, OrderExecutedWithPriceMessage &t);
void parseSystemEvent(const char* buf, SystemEventMessage &t);
void parseOrderCancelled(const char* buf, OrderCancelMessage &t);

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