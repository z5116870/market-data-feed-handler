#include <iostream>
#include "parse.h"
#include "helper.h"
#include "sequencer.h"
#include <bit>
#include <chrono>

// Logger for printing parsed messages
static const Logger logger = LogLevel::OFF;

// Parsing loop, run for each syscall to obtain data from socket receive buffer
 void parseMessage(const char* buf, const ssize_t &len) {
    ssize_t pos = 0;
    char type;

    // The "len" value is the length of bytes read by recv. This should be at most
    // 1472 bytes, so we must go through the buffer and parse each
    // ITCH message based on the first byte (messageType) which also tells us the message
    // size (this is static, determined by the ITCH protocol specification)
    while(pos < len) {
        type = buf[pos];
        switch(type) {
            case 'A': pos += parseTrade(buf + pos, tradeMsg); break;
            case 'P': pos += parseTrade(buf + pos, tradeMsg); break;
            case 'E': pos += parseOrderExecuted(buf + pos, orderExecutedMsg); break;
            case 'X': pos += parseOrderWithPrice(buf + pos, orderExecutedWithPriceMsg); break;
            case 'S': pos += parseSystemEvent(buf + pos, sysMsg); break;
            case 'C': pos += parseOrderCancelled(buf + pos, orderCancelMsg); break;
        }
        //std::cout << std::endl;
    }
}

ssize_t parseTrade(const char *buf, TradeMessage &t) {
    using namespace std::chrono;
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);
    // show latency

    // 3. Sequence number
    t.sequenceNumber = read4Bytes(buf, offset);

    // 4. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);
    
    // 5. Buy/Sell Indicator
    t.buySellIndicator = buf[offset++];
    
    // 6. Shares bought/buying
    t.shares = read4Bytes(buf, offset);
    
    // 7. Stock name
    std::memcpy(&t.stock, buf + offset, 8);
    t.stock[8] = '\0';
    offset += 8;
    
    // 8. Price
    t.price = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
    // Set last sequence number
    checkAndSetGlobalState(t.sequenceNumber);
    return MessageSize::Trade;
}

ssize_t parseOrderExecuted(const char *buf, OrderExecutedMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Sequence Number (4 bytes)
    t.sequenceNumber = read4Bytes(buf, offset);

    // 3. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 4. Executed shares
    t.executedShares = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
    // Check and set last sequence number
    checkAndSetGlobalState(t.sequenceNumber);
    return MessageSize::OrderExecuted;
}

ssize_t parseOrderWithPrice(const char *buf, OrderExecutedWithPriceMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Sequence Number (4 bytes)
    t.sequenceNumber = read4Bytes(buf, offset);

    // 4. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 5. Executed shares
    t.executedShares = read4Bytes(buf, offset);

    // 6. Printable
    t.printable = buf[offset++];
    
    // 7. Executed price
    t.executedPrice = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
    // Set last sequence number
    checkAndSetGlobalState(t.sequenceNumber);
    return MessageSize::OrderExecutedWithPrice;
}

ssize_t parseSystemEvent(const char *buf, SystemEventMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Sequence Number (4 bytes)
    t.sequenceNumber = read4Bytes(buf, offset);

    // 4. Executed shares
    t.eventCode = buf[offset++];

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
    // Set last sequence number
    checkAndSetGlobalState(t.sequenceNumber);
    return MessageSize::SystemEvent;
}

ssize_t parseOrderCancelled(const char *buf, OrderCancelMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 3. Sequence Number (4 bytes)
    t.sequenceNumber = read4Bytes(buf, offset);

    // 4. Order Ref Number (8 bytes)
    t.orderRefNumber = read8Bytes(buf, offset);

    // 5. Executed shares
    t.cancelledShares = read4Bytes(buf, offset);

    logger.log(t);
    // Get latency
    getDelta(t.timestamp);
    // Set last sequence number
    checkAndSetGlobalState(t.sequenceNumber);
    return MessageSize::OrderCancelled;
}

void TradeMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " sequenceNumber=" << sequenceNumber << " orderRefNumber=" << orderRefNumber << " shares=" << shares << " stock=" \
    << stock << " buysell=" << buySellIndicator << " price=" << price;
}

void OrderExecutedMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " sequenceNumber=" << sequenceNumber << " orderRefNumber=" << orderRefNumber << " executedShares=" << executedShares;
}

void OrderExecutedWithPriceMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " sequenceNumber=" << sequenceNumber << " orderRefNumber=" << orderRefNumber << " executedShares=" << executedShares \
    << " executedPrice=" << executedPrice << " printable=" << printable;
}

void SystemEventMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " sequenceNumber=" << sequenceNumber << " eventCode=[" << eventCode << "]";
}

void OrderCancelMessage::getRawLogImpl() const {
    std::cout << "[" << messageType << "] " << "timestamp=" << timestamp \
    << " sequenceNumber=" << sequenceNumber << " orderRefNumber=" << orderRefNumber << " cancelledShares=" << cancelledShares;
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