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
    resetStaticMessages();
    char buf[36];
    fillTradeBuffer(buf, 'A', 123456, 42, 999, 'B', 1000, "AAPL", 150);
    parseTrade(buf, tradeMsg);

    EXPECT_EQ(tradeMsg.messageType, 'A');
    EXPECT_EQ(tradeMsg.timestamp, 123456);
    EXPECT_EQ(tradeMsg.sequenceNumber, 42);
    EXPECT_EQ(tradeMsg.orderRefNumber, 999);
    EXPECT_EQ(tradeMsg.buySellIndicator, 'B');
    EXPECT_EQ(tradeMsg.shares, 1000);
    EXPECT_STREQ(tradeMsg.stock, "AAPL");
    EXPECT_EQ(tradeMsg.price, 150);
}

TEST(ParseOrderExecutedTest, CorrectParsing) {
    resetStaticMessages();
    char buf[23] = {'E', 0,0,0,0,0,1, 0,0,0,2, 0,0,0,0,0,0,0,0,0,0,5, 0,0,0,100};
    parseOrderExecuted(buf, orderExecutedMsg);
    EXPECT_EQ(orderExecutedMsg.messageType, 'E');
}

TEST(ParseOrderWithPriceTest, CorrectParsing) {
    resetStaticMessages();
    char buf[28] = {'X'};
    buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = 0; // Timestamp placeholder
    buf[7] = buf[8] = buf[9] = buf[10] = 1; // Sequence
    buf[11] = buf[12] = buf[13] = buf[14] = 0; // orderRef placeholder
    buf[15] = 'P'; buf[16] = 0; buf[17] = 0; buf[18] = 50; // executedShares + printable
    buf[19] = buf[20] = buf[21] = buf[22] = 10; // executedPrice
    parseOrderWithPrice(buf, orderExecutedWithPriceMsg);
    EXPECT_EQ(orderExecutedWithPriceMsg.messageType, 'X');
}

TEST(ParseSystemEventTest, CorrectParsing) {
    resetStaticMessages();
    char buf[12] = {'S', 0,0,0,0,0,1, 0,0,0,2, 'O'};
    parseSystemEvent(buf, sysMsg);
    EXPECT_EQ(sysMsg.messageType, 'S');
    EXPECT_EQ(sysMsg.eventCode, 'O');
}

TEST(ParseOrderCancelledTest, CorrectParsing) {
    resetStaticMessages();
    char buf[23] = {'C', 0,0,0,0,0,1, 0,0,0,2, 0,0,0,0,0,0,0,5, 0,0,0,50};
    parseOrderCancelled(buf, orderCancelMsg);
    EXPECT_EQ(orderCancelMsg.messageType, 'C');
    EXPECT_EQ(orderCancelMsg.cancelledShares, 50);
}

TEST(ParseMessageTest, MultipleMessagesBuffer) {
    resetStaticMessages();
    char buf[36+23]; // Trade + OrderExecuted
    fillTradeBuffer(buf, 'A', 123, 1, 1, 'B', 10, "AAPL", 150);
    std::memset(buf+36, 'E', 23); // placeholder for OrderExecuted
    parseMessage(buf, 36+23);
    EXPECT_EQ(tradeMsg.messageType, 'A');
    EXPECT_EQ(orderExecutedMsg.messageType, 'E');
}
