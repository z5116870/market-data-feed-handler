#include "../src/parse.h"
#include "../src/helper.h"
#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <arpa/inet.h>

#define TEST_CASE(name) std::cout << "[TEST]: " << name << "..."
#define PASS() std::cout << "***PASS***\n"
#define FAIL(msg) do { std::cerr << "!!FAIL!!: " << msg << std::endl; std::exit(1); } while(0)
void testParsesTradeMessage() {
    // Fake a single Trade message buffer
    std::cout << "=== RUNNING TEST PARSER TRADE ===\n";
    uint8_t buf[64] = {};
    size_t offset = 0;
    buf[offset++] = 'P';
    uint64_t ts = 12345;
    std::memcpy(buf + offset, &ts, 6); offset += 6;
    uint32_t seq = htonl(1);
    std::memcpy(buf + offset, &seq, 4); offset += 4;
    uint64_t ref = htobe64(42);
    std::memcpy(buf + offset, &ref, 8); offset += 8;
    buf[offset++] = 'B';
    uint32_t shares = htonl(100);
    std::memcpy(buf + offset, &shares, 4); offset += 4;
    std::memcpy(buf + offset, "AAPL\0\0\0\0", 8); offset += 8;
    uint32_t price = htonl(12345);
    std::memcpy(buf + offset, &price, 4); offset += 4;

    TradeMessage msg;
    parseTrade(reinterpret_cast<const char*>(buf), msg);

    if(msg.messageType != 'P') FAIL("Incorrect message type");
    if(msg.shares != 100) FAIL("Incorrect shares");
    if(msg.price != 12345) FAIL("Incorrect price");
    PASS();
}

int main() {
    testParsesTradeMessage();
}
