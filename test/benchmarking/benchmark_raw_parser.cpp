#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <cstring>
#include <chrono>
#include "../../src/parse.h"
#include "../../src/sequencer.h"
#define MULTICAST_IP "239.1.1.1"
#define PORT 30001
#define LOG(x) std::cout << x << std::endl
#define LOGREAD(x) std::cout << "READ " << x << " BYTES\n"

int main() {
    // 1. Get the interface name used for the multicast IP
    std::string nic = "enxc8a362d92729";
    if (nic.empty()) {
        std::cerr << "Failed to determine interface for muticast IP: " << MULTICAST_IP << std::endl;
        return 1;
    }

    std::cout << "Found interface: " << nic;

    // 2. Get the interface index
    uint32_t index = if_nametoindex(nic.c_str());
    if (index == 0) {
        std::cerr << "Failed to find index for interface: " << nic << std::endl;
        return 1;
    }

    // 3. Create a UDP socket for receiving a byte stream (raw frames at L2)
    // We will process this ourselves and completely bypass kernel network stack
    // to avoid the rt_offload_failed issue
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sockfd < 0) {
        perror("Failed to initialize a socket.\n");
        return 1;
    }

    // 4. Because we are working at L2, bind refers to the interface on which
    // we want to receive the L2 frames. This requires the use a of sockaddr_ll structure
    // as opposed to sockaddr_in used at L4
    sockaddr_ll socket_link_layer{};
    socket_link_layer.sll_family = AF_PACKET; // AF_PACKET tells the OS to use this socket for Ethernet frames, similar to AF_INET being for IPv4
    socket_link_layer.sll_protocol = htons(ETH_P_IP); // Filtering is applied to deliver only IPv4 encapsulated ethernet frames
    socket_link_layer.sll_ifindex = index; // Set the previously found interface index
    
    if (bind(sockfd, (sockaddr *)&socket_link_layer, sizeof(socket_link_layer)) < 0) {
        perror("Failed to bind socket to found NIC\n");
        return 1;
    }


    std::cout << "LISTENING FOR FRAMES ON " << nic << std::endl;

    GlobalState::timerIsRunning.store(true, std::memory_order_relaxed);
    std::thread gapTimerThread(gapTimer);

    uint32_t NUM_MESSAGES = 100000;
    alignas(64) char buf[2048];
    auto now = std::chrono::steady_clock::now();
    while (GlobalState::parsedMessages < NUM_MESSAGES) {
        ssize_t nbytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (nbytes < 0) {
            perror("Error receiving data, terminating.");
            break;
        }
        iphdr* ip_header = (iphdr*)(buf + 14);

        uint32_t dest_ip_addr = ip_header->daddr;
        if (dest_ip_addr != inet_addr(MULTICAST_IP)) continue; // Compare the binary network byte order of the dest addr in the IP packet and the known multicast IP

        // We need the IP header length to determine the offset of the UDP header (IP header length is variable from 20-60 bytes)
        // we cannot just conclude that there are no options and use 20 bytes, so we must find it through the header fields.
        // The header length is determined by the internet header length (IHL) field (4 bit) which gives us its length in 32 bit words (4 bytes)
        // so multiply this by 4 to get the length in bytes (using IPv6 would be much simpler here, as the header is a static 20 bytes)
        int ip_header_length = ip_header->ihl * 4;

        // Filter by protocol (must be UDP i.e. 17)
        if (ip_header->protocol != 17) continue;

        // Now get the UDP header and filter by the 30001 port
        udphdr* udp_header = (udphdr*)(buf + 14 + ip_header_length);
        int udp_port = udp_header->dest;
        if (udp_port != htons(PORT)) continue; 

        // FINALLY get a pointer to the UDP payload using basic pointer arithmetoc
        char *payload = buf + 14 + ip_header_length + 8;

        // And determine the size using the IP header. The IP payload size is equal to the total length field (16 bit) - IHL (4 bit) * 4, which we already have.
        // Then we can get the UDP payload size by taking away the UDP header from that value
        ssize_t payload_length = ntohs(ip_header->tot_len) - ip_header_length - 8;
        parseMessage(payload, payload_length);
        handleGapTimeout();
        fflush(stdout);
    }
    GlobalState::timerIsRunning.store(false, std::memory_order_relaxed);
    gapTimerThread.join();
    auto end = std::chrono::steady_clock::now();
    long long time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - now).count();
    std::chrono::duration<double> time_taken_sec = end - now;
    close(sockfd);

    // RESULTS
    std::cout << "=== RESULTS ===\n";
    printf("Messages parsed: %d\n", NUM_MESSAGES);
    printf("Messages lost: %u\n", GlobalState::lostMessages);
    printf("Messages received out of order: %u\n", GlobalState::outOfOrderMessages);
    printf("Messages recieved as duplicates: %u\n", GlobalState::duplicates);
    printf("Time taken: %lld\n", time_taken);
    printf("Time taken per message: %lld\n", time_taken/NUM_MESSAGES);
    printf("Throughput: %f messages/sec\n", NUM_MESSAGES / time_taken_sec.count());
}