#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "parse.h"
#define MULTICAST_IP "239.1.1.1"
#define PORT 30001
#define LOG(x) std::cout << x << std::endl
#define LOGREAD(x) std::cout << "READ " << x << " BYTES\n"

int main() {
    // 1. Create a UDP socket for receiving a byte stream
    // IPv4/Datagrams/UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Failed to initialize a socket.\n");
        return 1;
    }

    // 2. Set socket options at socket layer (SOL_SOCKET)
    // SO_REUSEADDR = let the same addr/port be reused by multiple sockets (multiple processes)
    int one = 1; // Set the value to 1, setsocketopt expects a void *, so cast it.
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

    // 3. Set binding information
    sockaddr_in listen_addr{};
    listen_addr.sin_family = AF_INET; // IPv4
    listen_addr.sin_port = htons(PORT); // htons conversion, to convert machine endianness to network byte order (MSB first i.e. big endian)
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind this socket to listen on all local IPs ()

    // 4. BIND
    if (bind(sockfd, (sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 5. Join the 239.1.1.1 multicast group
    ip_mreq mcastMship{};
    mcastMship.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP); // Convert the multicast IP to network byte order 
    mcastMship.imr_interface.s_addr = htonl(INADDR_ANY); // Apply the multicast filtering to ALL NICs, let the kernel pick the right one

    // Set the IP_ADD_MEMBERSHIP socket option, using the ip_mreq struct
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcastMship, sizeof(mcastMship)) < 0)
    {
        perror("Failed to set IP_ADD_MEMBERSHIP option at IP layer (IPPROTO_IP) on client socket.\n");
        return 1;
    }
    
    std::cout << "LISTENING FOR MULTICAST TRAFFIC ON " << MULTICAST_IP << std::endl;

    // 6. Receive traffic
    // align the buffer to the size of a cache
    alignas(64) char buf[1024];
    while (1) {
        // Each call to recv reads from the socket receive buffer populated by the kernel
        // which loads the UDP payload containing each ITCH message independently
        ssize_t nbytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (nbytes < 0) {
            perror("Error receiving data, terminating.");
            break;
        }
        // Now buf contains the bytes for the next ITCH message, so we can parse it
        parseMessage(buf, nbytes);
        fflush(stdout);
        std::cout << std::endl;
    }
    // 7. Leave the group and close
    setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mcastMship, sizeof(mcastMship));
    close(sockfd);
}