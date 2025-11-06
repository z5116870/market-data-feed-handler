#include <iostream>
#include "parse.h"
#include "helper.h"
#include <bit>
#define PARSE_LOG 1

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
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);
    
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
    std::cout << offset << "---\n";
    #ifdef PARSE_LOG
        std:: cout << t;
    #endif
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

    #ifdef PARSE_LOG
        std:: cout << t;
    #endif
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

    #ifdef PARSE_LOG
        std:: cout << t;
    #endif
}

void parseSystemEvent(const char *buf, SystemEventMessage &t) {
    size_t offset = 0;
    // 1. Message Type (1 byte)
    t.messageType = buf[offset++];

    // 2. Timestamp (6 bytes)
    t.timestamp = readTimestamp(buf, offset);

    // 4. Executed shares
    t.eventCode = buf[offset++];
    #ifdef PARSE_LOG
        std:: cout << t;
    #endif
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

    #ifdef PARSE_LOG
        std:: cout << t;
    #endif
}

std::ostream &operator<<(std::ostream &s, TradeMessage &t) {
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

std::ostream &operator<<(std::ostream &s, OrderExecutedMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order executed: " "[" << t.orderRefNumber << "]: " << \
    t.executedShares << " shares" << std::endl;
    return s;
}

std::ostream &operator<<(std::ostream &s, OrderExecutedWithPriceMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order executed with price Order ID: [" \
    << t.orderRefNumber << "]: " << t.executedShares << " @ " << t.executedPrice << std::endl;
    return s;
}

std::ostream &operator<<(std::ostream &s, SystemEventMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "*[" << out << "]" << " | " << (t.eventCode == 'O' ? "MARKET OPEN" : "MARKET CLOSE") \
    << "*\n";
    return s;
}

std::ostream &operator<<(std::ostream &s, OrderCancelMessage &t) {
    // Get timestamp in human readable form
    char out[20];
    nsToTimeStr(t.timestamp, out);

    s << "[" << out << "]" << " | " << "Order cancelled Order ID: [" << t.orderRefNumber << "] cancelled." << std::endl;
    return s;
}