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

// Helper: Fill buffer for SystemEventMessage
void fillSystemEventBuffer(char* buf, char type, uint64_t ts, uint32_t seq, char eventCode) {
    size_t offset = 0;
    buf[offset++] = type;
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    buf[offset++] = eventCode;
}

// Reset static messages
void resetStaticMessages() {
    tradeMsg = TradeMessage{};
    orderExecutedMsg = OrderExecutedMessage{};
    orderExecutedWithPriceMsg = OrderExecutedWithPriceMessage{};
    sysMsg = SystemEventMessage{};
    orderCancelMsg = OrderCancelMessage{};
}

// --- Integration Test ---
TEST(ParseIntegrationTest, MultipleMessageSequence) {
    resetStaticMessages();

    char buffer[36 + 23 + 12]; // Trade + OrderExecuted + SystemEvent

    // Fill Trade message
    fillTradeBuffer(buffer, 'A', 1000, 1, 101, 'B', 500, "AAPL", 150);
    // Fill OrderExecuted message right after Trade
    fillOrderExecutedBuffer(buffer + 36, 'E', 1001, 2, 101, 300);
    // Fill SystemEvent message after that
    fillSystemEventBuffer(buffer + 36 + 23, 'S', 1002, 3, 'O');

    // Parse entire buffer
    parseMessage(buffer, sizeof(buffer));

    // Verify Trade
    EXPECT_EQ(tradeMsg.messageType, 'A');
    EXPECT_EQ(tradeMsg.timestamp, 1000);
    EXPECT_EQ(tradeMsg.sequenceNumber, 1);
    EXPECT_EQ(tradeMsg.orderRefNumber, 101);
    EXPECT_EQ(tradeMsg.buySellIndicator, 'B');
    EXPECT_EQ(tradeMsg.shares, 500);
    EXPECT_STREQ(tradeMsg.stock, "AAPL");
    EXPECT_EQ(tradeMsg.price, 150);

    // Verify OrderExecuted
    EXPECT_EQ(orderExecutedMsg.messageType, 'E');
    EXPECT_EQ(orderExecutedMsg.timestamp, 1001);
    EXPECT_EQ(orderExecutedMsg.sequenceNumber, 2);
    EXPECT_EQ(orderExecutedMsg.orderRefNumber, 101);
    EXPECT_EQ(orderExecutedMsg.executedShares, 300);

    // Verify SystemEvent
    EXPECT_EQ(sysMsg.messageType, 'S');
    EXPECT_EQ(sysMsg.timestamp, 1002);
    EXPECT_EQ(sysMsg.sequenceNumber, 3);
    EXPECT_EQ(sysMsg.eventCode, 'O');
}
