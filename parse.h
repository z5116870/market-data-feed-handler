#pragma once
#include <cstddef>
#include <iostream>

// Parsing structs
struct Message {
    char        messageType;
    uint64_t    timestamp;
};

struct OrderMessage: Message {
    uint64_t    orderRefNumber;

};

struct TradeMessage: OrderMessage {
    char        buySellIndicator;
    uint32_t    shares;
    uint32_t    price;
    char        stock[8];

};

struct OrderExecutedMessage: OrderMessage {
    uint32_t    executedShares;
};

struct OrderExecutedWithPriceMessage: OrderMessage {
    char        printable;
    uint32_t    executedPrice;
    uint32_t    executedShares;
};

struct SystemEventMessage: Message {
    char        eventCode;
};

struct OrderCancelMessage: OrderMessage {
    uint32_t    cancelledShares;
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