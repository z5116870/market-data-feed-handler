#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include "../../src/parse.h"
#include "../../src/sequencer.h"
#include <thread>

#define MULTICAST_IP "239.1.1.1"
#define PORT 30001
#define LOG(x) std::cout << x << std::endl
#define LOGREAD(x) std::cout << "READ " << x << " BYTES\n"

// TURN OFF LOGGING
static const Logger logger = LogLevel::OFF;

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Failed to initialize a socket.\n");
        return 1;
    }

    int one = 1; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &one, sizeof(one)) < 0)
    {
        perror("Failed to set SO_REUSEADDR option at socket layer (SOL_SOCKET) on client socket.\n");
        return 1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void *) &one, sizeof(one)) < 0)
    {
        perror("Failed to set SO_REUSEPORT option at socket layer (SOL_SOCKET) on client socket.\n");
        return 1;
    }

    // Set larger receive buffer to reduce messages lost
    int rcvbuf = 4 * 1024 * 1024; // 4MB
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
        perror("Failed to set large receive buffer for parser socket");
    }

    sockaddr_in listen_addr{};
    listen_addr.sin_family = AF_INET; // IPv4
    listen_addr.sin_port = htons(PORT); // htons conversion, to convert machine endianness to network byte order (MSB first i.e. big endian)
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind this socket to listen on all local IPs ()

    if (bind(sockfd, (sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ip_mreq mcastMship{};
    mcastMship.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP); // Convert the multicast IP to network byte order 
    mcastMship.imr_interface.s_addr = htonl(INADDR_ANY); // Apply the multicast filtering to ALL NICs, let the kernel pick the right one

    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcastMship, sizeof(mcastMship)) < 0)
    {
        perror("Failed to set IP_ADD_MEMBERSHIP option at IP layer (IPPROTO_IP) on client socket.\n");
        return 1;
    }
    
    std::cout << "LISTENING FOR MULTICAST TRAFFIC ON " << MULTICAST_IP << std::endl;

    GlobalState::timerIsRunning.store(true, std::memory_order_relaxed);
    std::thread gapTimerThread(gapTimer);
    alignas(64) char buf[1500];
    uint32_t NUM_MESSAGES = 100000;
    auto now = std::chrono::steady_clock::now();
    while (GlobalState::parsedMessages < NUM_MESSAGES) {
        ssize_t nbytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (nbytes < 0) {
            perror("Error receiving data, terminating.");
            break;
        }
        parseMessage(buf, nbytes);

        fflush(stdout);
        // Check if the timer expired and if GlobalState::gapTimeout = true
        handleGapTimeout();
    }
    GlobalState::timerIsRunning.store(false, std::memory_order_relaxed);
    gapTimerThread.join();
    auto end = std::chrono::steady_clock::now();
    long long time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - now).count();
    std::chrono::duration<double> time_taken_sec = end - now;
    setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mcastMship, sizeof(mcastMship));
    close(sockfd);
    std::cout << "=== RESULTS ===\n";
    printf("Messages parsed: %d\n", NUM_MESSAGES);
    printf("Messages lost: %u\n", GlobalState::lostMessages);
    printf("Messages received out of order: %u\n", GlobalState::outOfOrderMessages);
    printf("Messages recieved as duplicates: %u\n", GlobalState::duplicates);
    printf("Time taken: %lld\n", time_taken);
    printf("Time taken per message: %lld\n", time_taken/NUM_MESSAGES);
    printf("Throughput: %f messages/sec\n", NUM_MESSAGES / time_taken_sec.count());
}