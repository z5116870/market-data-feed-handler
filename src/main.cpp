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
#include <sys/mman.h>
#include "parse.h"
#include "sequencer.h"
#define MULTICAST_IP "239.1.1.1"
#define PORT 30001
#define LOG(x) std::cout << x << std::endl
#define LOGREAD(x) std::cout << "READ " << x << " BYTES\n"

// PACKET_MMAP RING BUFFER CONSTS
constexpr unsigned int frame_size = 2048;
constexpr unsigned int num_of_frames = 4096;
constexpr unsigned int frames_per_block = 16;

int main() {
    // 1. Get the interface name used for the multicast IP
    std::string nic = "enxc8a362d92729";
    if (nic.empty()) {
        std::cerr << "Failed to determine interface for muticast IP: " << MULTICAST_IP << std::endl;
        return 1;
    }

    std::cout << "Found interface: " << nic << std::endl;

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

    // 4. PACKET_MMAP RING BUFFER SETUP
    // First up is creating a socket option to tell the kernel to write the frames from the bound NIC
    // (happens below) to the new PACKET_RX_RING
    tpacket_req req{};
    req.tp_frame_size = frame_size;
    req.tp_frame_nr = num_of_frames;
    req.tp_block_size = frames_per_block * frame_size;
    req.tp_block_nr = num_of_frames / frames_per_block;

    unsigned int mmap_len = frame_size * num_of_frames;

    if (setsockopt(sockfd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req)) < 0) {
        perror("Failed to create RX ring buffer on socket\n");
        return 1;
    }

    // 5. Memory map the shared buffer into the MDFH process address space
    void *ringPtr = mmap(
        nullptr,                    // Let the kernel choose any suitable VA in MDFH address space
        mmap_len,                   // Length of memory to map (exact size of the buffer)
        PROT_READ | PROT_WRITE,     // MDFH will read and write (altering the frames tp_status to TP_STATUS_KERNEL from TP_STATUS_USER)
        MAP_SHARED,                 // Indicates memory is shared between kernel and user space
        sockfd,                     // Pass in the shared buffer via the socket descriptor for the raw socket
        0                           // Use offset = 0 because we want to start from the beginning of the buffer
    );

    // 6. Because we are working at L2, bind refers to the interface on which
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

    // 7. Start timer thread for packet sequencer (for detecting losses when gaps opened in stream
    // due to out-of-order messages)
    GlobalState::timerIsRunning.store(true, std::memory_order_relaxed);
    std::thread gapTimerThread(gapTimer);

    // 8. Receive traffic
    // align the buffer to the size of a cache
    alignas(64) char buf[2048];

    // 9. Loop over the shared ring buffer in modulo pattern so we continuously iterate
     // Ensure the frame index is always within the frame count of the shared ring buffer
    uint32_t frame_num = 0;
    for(uint32_t frame_num = 0; ;frame_num = (frame_num + 1) % num_of_frames)
    {
        // Get the TPACKET frame for the current frame number and get its header
        auto *frame_header = (tpacket_hdr *)((uint8_t*)ringPtr + (frame_num * frame_size));
        if (frame_header->tp_status != TP_STATUS_USER) {
            // If the kernel has not yet readied this frame (i.e. status should be TP_STATUS_USER
            // then simply yield the CPU and let another process/thread take over
            // TODO: BE MORE SELFISH
            continue;
        }

        // Get the ethernet frame from the TPACKET frame
        // Add the offset of the ethernet header to the frame_header to get a pointer to the ethernet header
        char *buf = (char *)frame_header + frame_header->tp_mac;
        if (frame_header->tp_len < 14) {
            releaseFrame(frame_header);
            continue;
        }

        // Now buf contains the read ethernet frame, we must decode it and obtain the UDP payload
        // We dont need to parse the dest MAC because its already encoded in the dest IP (01:00:5e:01:01:01 => 239.1.1.1)
        // So we can skip the ethernet header (14 bytes) and go directly to the ip header
        iphdr* ip_header = (iphdr*)(buf + 14);

        // Now we can filter by the dest IP addr (should be 239.1.1.1)
        uint32_t dest_ip_addr = ip_header->daddr;
        if (dest_ip_addr != inet_addr(MULTICAST_IP)) {
            releaseFrame(frame_header);
            continue; // Compare the binary network byte order of the dest addr in the IP packet and the known multicast IP
        }
        // We need the IP header length to determine the offset of the UDP header (IP header length is variable from 20-60 bytes)
        // we cannot just conclude that there are no options and use 20 bytes, so we must find it through the header fields.
        // The header length is determined by the internet header length (IHL) field (4 bit) which gives us its length in 32 bit words (4 bytes)
        // so multiply this by 4 to get the length in bytes (using IPv6 would be much simpler here, as the header is a static 20 bytes)
        int ip_header_length = ip_header->ihl * 4;

        // Filter by protocol (must be UDP i.e. 17)
        if (ip_header->protocol != 17) {
            releaseFrame(frame_header);
            continue;
        }

        // Now get the UDP header and filter by the 30001 port
        udphdr* udp_header = (udphdr*)(buf + 14 + ip_header_length);
        int udp_port = udp_header->dest;
        if (udp_port != htons(PORT)) {
            releaseFrame(frame_header); 
            continue; 
        }

        // FINALLY get a pointer to the UDP payload using basic pointer arithmetoc
        char *payload = buf + 14 + ip_header_length + 8;

        // And determine the size using the IP header. The IP payload size is equal to the total length field (16 bit) - IHL (4 bit) * 4, which we already have.
        // Then we can get the UDP payload size by taking away the UDP header from that value
        ssize_t payload_length = ntohs(ip_header->tot_len) - ip_header_length - 8;
        parseMessage(payload, payload_length);
        fflush(stdout);

        releaseFrame(frame_header);
    }

    // 8. Stop the timer thread
    GlobalState::timerIsRunning.store(false, std::memory_order_relaxed);
    gapTimerThread.join();
    munmap(ringPtr, mmap_len); // Unmap the shared memory to release it
    close(sockfd);
}