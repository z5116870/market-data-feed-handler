// Create UDP replay server, reads bytes from itch_data.bin and sends to 239.1.1.1 (same multicast address)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <chrono>

constexpr uint32_t PORT = 30001;
constexpr uint32_t READ_BUFFER_SIZE = 3000000;
constexpr uint32_t SEND_BUFFER_SIZE = 1472;
constexpr char MULTICAST_IP[] = "239.1.1.1";

// ENUM for message sizes
enum MessageSize {
    Trade = 36,
    OrderExecuted = 23,
    OrderExecutedWithPrice = 28,
    SystemEvent = 12,
    OrderCancelled = 23
};

// Get size of each ITCH message, so we can pack them into the 1472 byte buffer properly
// If we were to blindly send 1472 bytes at a time, we risk destroying message boundaries
size_t getMessageSize(const char &msgType) {
    switch (msgType){
        case 'A': return MessageSize::Trade;
        case 'P': return MessageSize::Trade;
        case 'E': return MessageSize::OrderExecuted;
        case 'X': return MessageSize::OrderExecutedWithPrice;
        case 'S': return MessageSize::SystemEvent;
        case 'C': return MessageSize::OrderCancelled;
        default: return 0;
    }
}

int main() {
    std::cout << "=== ITCH MESSAGE UDP REPLAY SERVER ===\n";
    // Create socket and bind to 127.0.0.1
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 1) {
        perror("Error creating socket.\n");
        return 1;
    }
    
    std::cout << "Socket initialised.\n";
    // Only allow one hop for multicast traffic out of this socket
    int ttl = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("Error setting UDP socket options.\n");
        return 1;
    }

    std::cout << "Socket options set.\n";
    // Create socket address for multicast IP
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    int rcode = inet_pton(AF_INET, MULTICAST_IP, &dest.sin_addr);
    if (rcode == 0) {
        printf("%s is an Invalid string.\n", MULTICAST_IP);
        return 1;
    } else if (rcode < 0) {
        perror("Failed to create binary form of IPv4 multicast address.\n");
    }

    std::cout << "Multicast destination address set.\n";

    // Read from the file (approx 2.4MB, allocate 3MB)
    char* fileBuf = new char[READ_BUFFER_SIZE];

    // Read as bytes (std::ios::binary) and set read pointer position to end of file (std::ios::ate)
    std::ifstream a("itch_data.bin", std::ios::binary | std::ios::ate);

    // Now that the read pointer is at the end of the file, call tellg() to tell us its position within the input stream
    // i.e. number of bytes from beginning to end = size of file
    size_t nbytes = a.tellg();

    // return read pointer to beginning, equivalent of seekg(0, std::ios::beg) = move read pointer to 0 bytes from beginning
    a.seekg(0);
    a.read(fileBuf, nbytes);

    // Buffer for sending messages over UDP (1472 bytes is max size of UDP payload, 20 byte IP header, 8 byte UDP header)
    char msgBuf[SEND_BUFFER_SIZE];

    size_t filePos = 0; // byte pointer for file buffer (fileBuf)
    size_t msgPos = 0; // byte pointer for message buffer (msgBuf)
    size_t msgSize = 0; // variable for message size

    std::cout << "Running UDP replay server...\n";
    // Send bytes to the multicast IP indefinitely
    while (1) {
        // Read the entire files bytes
        while (filePos < nbytes) {
            // If the next message would exceed 1472, break this loop
            // and flush the buffer (send the data)
            msgSize = getMessageSize(fileBuf[filePos]); 
            if (msgPos + msgSize > SEND_BUFFER_SIZE) break;

            // Otherwise copy the data from the file into the buffer,
            // the incrememnt the msg and file positions by the copied
            // ITCH messages' size
            memcpy(msgBuf + msgPos, fileBuf + filePos, msgSize);
            msgPos += msgSize;
            filePos += msgSize;
        }

        // If we have reached the end of the fileBuf, reset the loop
        if (filePos == nbytes) {
            filePos = 0;
            continue;
        }

        // We only get here if the msgBuf was going to be exceeded, so now
        // we need to flush the buffer and reset the msgPos and start overwriting
        // the buffer
        ssize_t bytesSent = sendto(sock, msgBuf, msgPos, 0, (sockaddr*) &dest, sizeof(dest));

        
        if (bytesSent < 0) {
            perror("Can't send bytes");
        }

        msgPos = 0;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    // Close the file
    a.close();
    // Free the heap array used to read the file
    delete[] fileBuf;
}

