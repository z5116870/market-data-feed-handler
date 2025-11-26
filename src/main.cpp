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
#include <poll.h>
#include "parse.h"
#include "sequencer.h"
#define MULTICAST_IP "239.1.1.1"
#define PORT 30001
#define LOG(x) std::cout << x << std::endl
#define LOGREAD(x) std::cout << "READ " << x << " BYTES\n"

// PACKET_MMAP RING BUFFER CONSTS
constexpr unsigned int BLOCK_SIZE = 131072;
constexpr unsigned int FRAME_SIZE = 2048;
constexpr unsigned int BLOCK_NR = 64;
constexpr unsigned int FRAME_NR = (BLOCK_NR * BLOCK_SIZE) / FRAME_SIZE;

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

    // 4. PACKET_MMAP RING BUFFER SETUP (TPACKET_V3)
    // First up is creating a socket option to tell the kernel to write the frames from the bound NIC
    // (happens below) to the new PACKET_RX_RING (similar to TPACKET_V1 except we need to set some block parameters)
    tpacket_req3 req{};
    req.tp_frame_size = FRAME_SIZE;
    req.tp_frame_nr = FRAME_NR;
    req.tp_block_size = BLOCK_SIZE;
    req.tp_block_nr = BLOCK_NR;
    req.tp_retire_blk_tov = 0;
    req.tp_sizeof_priv = 0;

    if (setsockopt(sockfd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req)) < 0) {
        perror("Failed to create RX ring buffer on socket\n");
        return 1;
    }

    int v = TPACKET_V3;

    // 5. Enable TPACKET_V3 by setting the version as a socket option at the SOL_PACKET layer
    setsockopt(sockfd, SOL_PACKET, PACKET_VERSION, &v, sizeof(v));

    // 6. Memory map the shared buffer into the MDFH process address space
     unsigned int mmap_len = BLOCK_SIZE * BLOCK_NR;
    void *ringPtr = mmap(
        nullptr,                    // Let the kernel choose any suitable VA in MDFH address space
        mmap_len,                   // Length of memory to map (exact size of the buffer)
        PROT_READ | PROT_WRITE,     // MDFH will read and write (altering the frames tp_status to TP_STATUS_KERNEL from TP_STATUS_USER)
        MAP_SHARED,                 // Indicates memory is shared between kernel and user space
        sockfd,                     // Pass in the shared buffer via the socket descriptor for the raw socket
        0                           // Use offset = 0 because we want to start from the beginning of the buffer
    );

    if (ringPtr == MAP_FAILED) {
        perror("Failed (mmap()) to create shared ring buffer between user space and kernel\n");
        return 1;
    }

    // 7. Because we are working at L2, bind refers to the interface on which
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

    // 8. Loop over the shared ring buffer in modulo pattern so we continuously iterate
     // Ensure the frame index is always within the frame count of the shared ring buffer
    uint32_t mcast_ip = inet_addr(MULTICAST_IP);
    uint16_t dest_port = htons(PORT);
    for(uint32_t block_idx = 0; ;block_idx = (block_idx + 1) % BLOCK_NR)
    {
        // Get the TPACKET_V3 block pointer 
        tpacket_block_desc *block_ptr = (tpacket_block_desc *)((uint8_t *)ringPtr + (block_idx * BLOCK_SIZE));
        // If the kernel has not yet readied this block, simply check the next block by continuing the loop
        if (!(block_ptr->hdr.bh1.block_status & TP_STATUS_USER)) {
            
            continue;
        }
        
        // Use the block metadata to get a pointer to the first TPACKET_V3 packet in the block
        uint32_t num_pkts = block_ptr->hdr.bh1.num_pkts;
        uint32_t offset_to_first_pkt = block_ptr->hdr.bh1.offset_to_first_pkt;

        // Using simple pointer arithmetic, add the offset of the first packet to the block pointer to obtains
        // the pointer to the first packet
        tpacket3_hdr* current_packet = (tpacket3_hdr *)((uint8_t *)block_ptr + offset_to_first_pkt);

        // Iterate through every packet in the block
        for (uint32_t i = 0; i < num_pkts; i++) {
            // The tpacket3_hdr struct has extended fields compared to V1, one of which is the next tp_next_offset,
            // which gives the offset of the next packet. We can use this to prefetch the next packet and load it into 
            // the L1 cache so it is ready for processing immediately after this one, so no cycles are wasted. 
            __builtin_prefetch((uint8_t*) current_packet + current_packet->tp_next_offset);

            // ---------------------------------------------------------------------------------
            // NOW WE PROCESS THE PACKET JUST AS WITH TPACKETV1
            // ON A FAIL CONDITION, WE MOVE TO THE NEXT PACKET AND INCREMENT THE COUNTER
            // ---------------------------------------------------------------------------------
             // Get the ethernet frame from the TPACKET frame
            // Add the offset of the ethernet header to the frame_header to get a pointer to the ethernet header
            char *buf = (char *)current_packet + current_packet->tp_mac;

            // Now buf contains the read ethernet frame, we must decode it and obtain the UDP payload
            // We dont need to parse the dest MAC because its already encoded in the dest IP (01:00:5e:01:01:01 => 239.1.1.1)
            // So we can skip the ethernet header (14 bytes) and go directly to the ip header
            iphdr* ip_header = (iphdr*)(buf + 14);

            // Now we can filter by the dest IP addr (should be 239.1.1.1)
            // Compare the binary network byte order of the dest addr in the IP packet and the known multicast IP
            uint32_t dest_ip_addr = ip_header->daddr;
            if (dest_ip_addr != mcast_ip) {
                current_packet = (tpacket3_hdr *)((uint8_t*) current_packet + current_packet->tp_next_offset);
                continue; 
            }
            // We need the IP header length to determine the offset of the UDP header (IP header length is variable from 20-60 bytes)
            // we cannot just conclude that there are no options and use 20 bytes, so we must find it through the header fields.
            // The header length is determined by the internet header length (IHL) field (4 bit) which gives us its length in 32 bit words (4 bytes)
            // so multiply this by 4 to get the length in bytes (using IPv6 would be much simpler here, as the header is a static 20 bytes)
            int ip_header_length = ip_header->ihl * 4;

            // Filter by protocol (must be UDP i.e. 17)
            if (ip_header->protocol != 17) {
                current_packet = (tpacket3_hdr *)((uint8_t*) current_packet + current_packet->tp_next_offset);
                continue;
            }

            // Now get the UDP header and filter by the 30001 port
            udphdr* udp_header = (udphdr*)(buf + 14 + ip_header_length);
            int udp_port = udp_header->dest;
            if (udp_port != dest_port) {
                current_packet = (tpacket3_hdr *)((uint8_t*) current_packet + current_packet->tp_next_offset); 
                continue; 
            }

            // FINALLY get a pointer to the UDP payload using basic pointer arithmetoc
            char *payload = buf + 14 + ip_header_length + 8;

            // And determine the size using the IP header. The IP payload size is equal to the total length field (16 bit) - IHL (4 bit) * 4, which we already have.
            // Then we can get the UDP payload size by taking away the UDP header from that value
            ssize_t payload_length = ntohs(ip_header->tot_len) - ip_header_length - 8;
            parseMessage(payload, payload_length);
            handleGapTimeout();
            current_packet = (tpacket3_hdr *)((uint8_t*) current_packet + current_packet->tp_next_offset);
        }

        release_block(block_ptr);
    }

    // 8. Stop the timer thread
    GlobalState::timerIsRunning.store(false, std::memory_order_relaxed);
    gapTimerThread.join();
    munmap(ringPtr, mmap_len); // Unmap the shared memory to release it
    close(sockfd);
}