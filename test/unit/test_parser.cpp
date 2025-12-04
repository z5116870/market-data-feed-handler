#include "gtest/gtest.h"
#include "parse.h"
#include <cstring>

// Helper: Fill buffer for TradeMessage
void fillTradeBuffer(char* buf, char type, uint64_t ts, uint32_t seq, uint64_t orderRef, char buySell, uint32_t shares, const char* stock, uint32_t price) {
    size_t offset = 0;
    buf[offset++] = type;
    // Timestamp: 6 bytes
    for (int i = 5; i >= 0; --i) buf[offset++] = (ts >> (8*i)) & 0xFF;
    // Sequence number: 4 bytes
    for (int i = 3; i >= 0; --i) buf[offset++] = (seq >> (8*i)) & 0xFF;
    // Order ref number: 8 bytes
    for (int i = 7; i >= 0; --i) buf[offset++] = (orderRef >> (8*i)) & 0xFF;
    buf[offset++] = buySell;
    for (int i = 3; i >= 0; --i) buf[offset++] = (shares >> (8*i)) & 0xFF;
    std::memset(buf + offset, 0, 8);
    std::strncpy(buf + offset, stock, 8);
    offset += 8;
    for (int i = 3; i >= 0; --i) buf[offset++] = (price >> (8*i)) & 0xFF;
}

// Reset the static messages before each test
void resetStaticMessages() {
    tradeMsg = TradeMessage{};
    orderExecutedMsg = OrderExecutedMessage{};
    orderExecutedWithPriceMsg = OrderExecutedWithPriceMessage{};
    sysMsg = SystemEventMessage{};
    orderCancelMsg = OrderCancelMessage{};
}

// --- Tests ---

TEST(ParseTradeTest, CorrectParsing) {
    // GIVEN a input TradeMessage
    resetStaticMessages();
    char buf[36];
    fillTradeBuffer(buf, 'A', 123456, 42, 999, 'B', 1000, "AAPL", 150);

    // WHEN we parse it
    parseTrade(buf, tradeMsg);

    // THEN we expect the parsed struct to be set correctly
    EXPECT_EQ(tradeMsg.messageType, 'A');
    EXPECT_EQ(tradeMsg.timestamp, 123456);
    EXPECT_EQ(tradeMsg.sequenceNumber, 42);
    EXPECT_EQ(tradeMsg.orderRefNumber, 999);
    EXPECT_EQ(tradeMsg.buySellIndicator, 'B');
    EXPECT_EQ(tradeMsg.shares, 1000);
    EXPECT_STREQ(tradeMsg.stock, "AAPL");
    EXPECT_EQ(tradeMsg.price, 150);
}
