#include "gtest/gtest.h"
#include "parse.h"
#include <cstring>

// Helper: Fill buffer for TradeMessage
void fillTradeBuffer(char* buf, char type, uint64_t ts, uint32_t seq, uint64_t orderRef, char buySell, uint32_t shares, const char* stock, uint32_t price) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    for (int i = 7; i >= 0; --i) buf[offset++] = (orderRef >> (8*i)) & 0xFF;
    buf[offset++] = buySell;
    for (int i = 3; i >= 0; --i) buf[offset++] = (shares >> (8*i)) & 0xFF;
    std::memset(buf + offset, 0, 8);
    std::strncpy(buf + offset, stock, 8);
    offset += 8;
    for (int i = 3; i >= 0; --i) buf[offset++] = (price >> (8*i)) & 0xFF;
}

// Helper: Fill buffer for OrderExecutedMessage
void fillOrderExecutedBuffer(char* buf, char type, uint64_t ts, uint32_t seq, uint64_t orderRef, uint32_t executedShares) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    for (int i = 7; i >= 0; --i) buf[offset++] = (orderRef >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (executedShares >> (8*i)) & 0xFF;
}

// Helper: Fill buffer for OrderExecutedWithPriceMessage
void fillOrderWithPriceBuffer(char* buf, char type, uint64_t ts, uint32_t seq, uint64_t orderRef, uint32_t executedShares, char printable, uint32_t executedPrice) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    for (int i = 7; i >= 0; --i) buf[offset++] = (orderRef >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (executedShares >> (8*i)) & 0xFF;
    buf[offset++] = printable;
    for (int i = 3; i >= 0; --i) buf[offset++] = (executedPrice >> (8*i)) & 0xFF;
}

// Helper: Fill buffer for SystemEventMessage
void fillSystemEventBuffer(char* buf, char type, uint64_t ts, uint32_t seq, char eventCode) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    buf[offset++] = eventCode;
}

// Helper: Fill buffer for OrderCancelMessage
void fillOrderCancelBuffer(char* buf, char type, uint64_t ts, uint32_t seq, uint64_t orderRef, uint32_t cancelledShares) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    for (int i = 7; i >= 0; --i) buf[offset++] = (orderRef >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (cancelledShares >> (8*i)) & 0xFF;
}

// Reset all static messages
void resetStaticMessages() {
    tradeMsg = TradeMessage{};
    orderExecutedMsg = OrderExecutedMessage{};
    orderExecutedWithPriceMsg = OrderExecutedWithPriceMessage{};
    sysMsg = SystemEventMessage{};
    orderCancelMsg = OrderCancelMessage{};
}

TEST(ParseIntegrationTest, FullMessageSequence) {
    resetStaticMessages();

    const size_t totalSize = 36 + 23 + 28 + 12 + 23 + 36; // Trade + OrderExecuted + OrderWithPrice + System + Cancel + Trade(P)
    char buffer[totalSize];

    size_t offset = 0;

    // Trade 'A'
    fillTradeBuffer(buffer + offset, 'A', 1000, 1, 101, 'B', 500, "AAPL", 150);
    offset += 36;

    // OrderExecuted 'E'
    fillOrderExecutedBuffer(buffer + offset, 'E', 1001, 2, 101, 300);
    offset += 23;

    // OrderExecutedWithPrice 'X'
    fillOrderWithPriceBuffer(buffer + offset, 'X', 1002, 3, 102, 200, 'Y', 155);
    offset += 28;

    // SystemEvent 'S'
    fillSystemEventBuffer(buffer + offset, 'S', 1003, 4, 'O');
    offset += 12;

    // OrderCancel 'C'
    fillOrderCancelBuffer(buffer + offset, 'C', 1004, 5, 103, 150);
    offset += 23;

    // Trade 'P' (alternate trade type)
    fillTradeBuffer(buffer + offset, 'P', 1005, 6, 104, 'S', 250, "MSFT", 250);
    offset += 36;

    // Parse entire buffer
    parseMessage(buffer, offset);

    // --- Verify Trade 'A' ---
    EXPECT_EQ(tradeMsg.messageType, 'P'); // Last Trade in buffer overwrites static struct
    EXPECT_EQ(tradeMsg.timestamp, 1005);
    EXPECT_EQ(tradeMsg.sequenceNumber, 6);
    EXPECT_EQ(tradeMsg.orderRefNumber, 104);
    EXPECT_EQ(tradeMsg.buySellIndicator, 'S');
    EXPECT_EQ(tradeMsg.shares, 250);
    EXPECT_STREQ(tradeMsg.stock, "MSFT");
    EXPECT_EQ(tradeMsg.price, 250);

    // Verify OrderExecuted
    EXPECT_EQ(orderExecutedMsg.messageType, 'E');
    EXPECT_EQ(orderExecutedMsg.timestamp, 1001);
    EXPECT_EQ(orderExecutedMsg.sequenceNumber, 2);
    EXPECT_EQ(orderExecutedMsg.orderRefNumber, 101);
    EXPECT_EQ(orderExecutedMsg.executedShares, 300);

    // Verify OrderExecutedWithPrice
    EXPECT_EQ(orderExecutedWithPriceMsg.messageType, 'X');
    EXPECT_EQ(orderExecutedWithPriceMsg.timestamp, 1002);
    EXPECT_EQ(orderExecutedWithPriceMsg.sequenceNumber, 3);
    EXPECT_EQ(orderExecutedWithPriceMsg.orderRefNumber, 102);
    EXPECT_EQ(orderExecutedWithPriceMsg.executedShares, 200);
    EXPECT_EQ(orderExecutedWithPriceMsg.executedPrice, 155);
    EXPECT_EQ(orderExecutedWithPriceMsg.printable, 'Y');

    // Verify SystemEvent
    EXPECT_EQ(sysMsg.messageType, 'S');
    EXPECT_EQ(sysMsg.timestamp, 1003);
    EXPECT_EQ(sysMsg.sequenceNumber, 4);
    EXPECT_EQ(sysMsg.eventCode, 'O');

    // Verify OrderCancel
    EXPECT_EQ(orderCancelMsg.messageType, 'C');
    EXPECT_EQ(orderCancelMsg.timestamp, 1004);
    EXPECT_EQ(orderCancelMsg.sequenceNumber, 5);
    EXPECT_EQ(orderCancelMsg.orderRefNumber, 103);
    EXPECT_EQ(orderCancelMsg.cancelledShares, 150);
}
