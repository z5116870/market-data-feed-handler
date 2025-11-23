#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <arpa/inet.h>
#include <chrono>
#include "parse.h"

// Convert network to host order 64bit (equivalent of ntohll)
inline uint64_t ntohll(uint64_t &networkOrder) {
    uint8_t *p = reinterpret_cast<uint8_t*>(&networkOrder);
    uint64_t hostOrder = 0;
    for(int i = 0; i < 8; i++)
        hostOrder = hostOrder << 8 | p[i];
    return hostOrder;
    
}

inline uint64_t readTimestamp(const char* buf, size_t &offset) {
    uint64_t tmp = 0;
    // first 2 MSBs are empty (all 0), then store the timestamp value
    std::memcpy(reinterpret_cast<char*>(&tmp) + 2, buf + offset, 6); // upper 6 bytes
    offset += 6;
    return ntohll(tmp); // convert to host byte order
}


// Similar to readTimestamp, but this is for any number of 8 bytes
inline uint64_t read8Bytes(const char* buf, size_t &offset) {
    uint64_t tmp = 0;
    std::memcpy(reinterpret_cast<char*>(&tmp), buf + offset, 8);
    offset += 8;
    return ntohll(tmp); // convert to host byte order
} 

// Same but for 4 bytes
inline uint32_t read4Bytes(const char* buf, size_t &offset) {
    uint32_t tmp = 0;
    std::memcpy(reinterpret_cast<char*>(&tmp), buf + offset, 4);
    offset += 4;
    return ntohl(tmp); // convert to host byte order
} 

// Same function from server side (itch message generator)
inline void getDelta(uint64_t timestamp) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto midnight = floor<days>(now);
    auto since_midnight = now - midnight;
    uint64_t nano = duration_cast<nanoseconds>(since_midnight).count();
    uint64_t delta_ns = nano - timestamp;
    std::cout << " | " << delta_ns << " ns";
}

inline void nsToTimeStr(uint64_t ns_since_midnight, char *out) {
    // 1. Extract total seconds and remaining nanoseconds
    uint64_t total_sec = ns_since_midnight / 1'000'000'000ULL;
    uint32_t nsec = ns_since_midnight - total_sec * 1'000'000'000ULL;

    // 2. Compute hours, minutes, seconds
    uint64_t hours = total_sec / 3600;
    uint64_t rem = total_sec - hours * 3600;
    uint64_t minutes = rem / 60;
    uint64_t seconds = rem - minutes * 60;

    // 3. Fill char array manually for maximum speed
    out[0] = '0' + (hours / 10);
    out[1] = '0' + (hours % 10);
    out[2] = ':';
    out[3] = '0' + (minutes / 10);
    out[4] = '0' + (minutes % 10);
    out[5] = ':';
    out[6] = '0' + (seconds / 10);
    out[7] = '0' + (seconds % 10);
    out[8] = '.';

    // Nanoseconds (9 digits)
    out[17] = '0' + (nsec % 10); nsec /= 10;
    out[16] = '0' + (nsec % 10); nsec /= 10;
    out[15] = '0' + (nsec % 10); nsec /= 10;
    out[14] = '0' + (nsec % 10); nsec /= 10;
    out[13] = '0' + (nsec % 10); nsec /= 10;
    out[12] = '0' + (nsec % 10); nsec /= 10;
    out[11] = '0' + (nsec % 10); nsec /= 10;
    out[10] = '0' + (nsec % 10); nsec /= 10;
    out[9]  = '0' + (nsec % 10);

    out[18] = '\0'; // Null terminator
}