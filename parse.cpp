#include <iostream>
#include "parse.h"
#include "helper.h"
#include <bit>
#include <chrono>

// Logger for printing parsed messages
static const Logger logger = LogLevel::RAW;

// Parsing loop, run for each syscall to obtain data from socket receive buffer
// *TODO* 
 void parseMessage(const char* buf, size_t len) {
    char type = buf[0];
    switch(type) {
        case 'A': parseTrade(buf, tradeMsg); break;
        case 'P': parseTrade(buf, tradeMsg); break;
        case 'E': parseOrderExecuted(buf, orderExecutedMsg); break;
        case 'X': parseOrderWithPrice(buf, orderExecutedWithPriceMsg); break;
        case 'S': parseSystemEvent(buf, sysMsg); break;
        case 'C': parseOrderCancelled(buf, orderCancelMsg); break;
    }

}

void parseTrade(const char *buf, TradeMessage &t) {
    using namespace std::chrono;
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);
    // show latency

    // 3. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);
    
    // 4. Buy/Sell Indicator
    t.buySellIndicator = buf[offset++];
    
    // 5. Shares bought/buying
    t.shares = read4Bytes(buf, offset);
    
    // 6. Stock name
    std::memcpy(&t.stock, buf + offset, 8);
    t.stock[8] = '\0';
    offset += 8;
    
    // 7. Price
    t.price = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
}

void parseOrderExecuted(const char *buf, OrderExecutedMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 4. Executed shares
    t.executedShares = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
}

void parseOrderWithPrice(const char *buf, OrderExecutedWithPriceMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 4. Executed shares
    t.executedShares = read4Bytes(buf, offset);

    // 5. Printable
    t.printable = buf[offset++];
    
    // 6. Executed price
    t.executedPrice = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
}

void parseSystemEvent(const char *buf, SystemEventMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 4. Executed shares
    t.eventCode = buf[offset++];

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
}

void parseOrderCancelled(const char *buf, OrderCancelMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 4. Executed shares
    t.cancelledShares = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
}

void TradeMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " orderRefNumber=" << orderRefNumber << " shares=" << shares << " stock=" \
    << stock << " buysell=" << buySellIndicator << " price=" << price;
}

void OrderExecutedMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " orderRefNumber=" << orderRefNumber << " executedShares=" << executedShares;
}

void OrderExecutedWithPriceMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " orderRefNumber=" << orderRefNumber << " executedShares=" << executedShares \
    << " executedPrice=" << executedPrice << " printable=" << printable;
}

void SystemEventMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " eventCode=[" << eventCode << "]";
}

void OrderCancelMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " orderRefNumber=" << orderRefNumber << " cancelledShares=" << cancelledShares;
}

std::ostream &operator<<(std::ostream &s, const TradeMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " \
        << (t.messageType == 'A' ? "Order Added: " : "Trade: ") \
        << "[" << t.orderRefNumber << "]: " << t.shares << " of $" \
        << t.stock << " to " << (t.buySellIndicator == 'B' ? "Buy " : "Sell ") \
        << "@ " << t.price <<  std::endl;
    return s;
}

std::ostream &operator<<(std::ostream &s, const OrderExecutedMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order executed: " "[" << t.orderRefNumber << "]: " << \
    t.executedShares << " shares" << std::endl;
    return s;
}

std::ostream &operator<<(std::ostream &s, const OrderExecutedWithPriceMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order executed with price Order ID: [" \
    << t.orderRefNumber << "]: " << t.executedShares << " @ " << t.executedPrice << std::endl;
    return s;
}

std::ostream &operator<<(std::ostream &s, const SystemEventMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << (t.eventCode == 'O' ? "*MARKET OPEN" : "*MARKET CLOSE") \
    << "*\n";
    return s;
}

std::ostream &operator<<(std::ostream &s, const OrderCancelMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order cancelled Order ID: [" << t.orderRefNumber << "] cancelled." << std::endl;
    return s;
}