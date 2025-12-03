# market-data-feed-handler
This project is a C++ application designed to parse exchange market feeds in real time via UDP multicast traffic. To be used in conjunction with [itch-message-generator](https://github.com/z5116870/itch-message-generator/) which runs parallel to this and provides the market feed. Subscription to real ITCH feeds from real exchanges isn't possible for me :(

## Key Features

- **Zero-copy packet processing** — leverages packet mmap and direct buffer access to avoid unnecessary data copies from NIC to user code.  
- **Multithreaded parsing architecture** — supports parallel parsing pipelines to maximise throughput on multicore systems.  
- **Out-of-order / packet-loss / duplication detection** — tracks sequence metadata to detect reordering, missing packets, or duplicates.  
- **Minimal dependencies & lean C++ design** — emphasises performance, memory alignment, and efficiency; no heavy runtime dependencies.  
- **Configurable for real-world feeds** — can be adapted to various exchange protocols over UDP multicast.

## Motivation & Use Cases

This project was developed to:  

- Demonstrate low-level, high-performance systems programming in C++.  
- Provide a foundation for market data ingestion pipelines in trading, quant, or real-time analytics environments.  
- Serve as a portfolio piece to showcase architectural and performance-conscious coding skills relevant to high frequency trading (HFT), market data handling, and latency-sensitive systems.  

## Directory Layout

```
market-data-feed-handler/
├── src/            # Source code (parsing logic, main, etc.)
├── test/           # Tests and sample data for parsing / packet handling
├── screenshots/    # Example outputs, logs, benchmark results or diagrams
├── .gitignore
└── README.md
```

## Design & Implementation Notes

- **Zero-copy parsing:** Uses `mmap`‑ed RX buffer (or similar shared memory buffer) to avoid data copying; parsing reads directly from NIC buffer to parsed data structures.  
- **Thread-per‑pipeline model:** Dedicated threads for each parsing queue (lock-free SPSC) to decouple IO, parsing, and data sequencing — reduces contention and maximises CPU utilisation on multicore machines.  
- **Sequencer logic:** Maintains packet ordering metadata per feed, detects out-of-order, lost, or duplicate packets — enabling robust feed consumption even under high network load or packet churn.  
- **Cache-aware data structures:** Parsing results and internal buffers aligned to cache-line boundaries; where possible uses contiguous memory layouts to improve CPU cache utilisation and reduce latency.  

## Why This Project Matters
I have completed this project with the intention of testing the limits of my knowledge on C++, computer microarchitecture, operating systems, networking and concurrency. It has been done in conjunction with studying the aforementioned topics and with the aim of pivoting my career into the world of quantitative finance. I believe that this project showcases the following qualities:
- Strong mastery of systems programming and C++.  
- Understanding of network I/O, concurrency primitives, and performance optimisation.  
- Ability to design modular, maintainable, and efficient code for demanding real-time constraints.  
- Self-driven initiative and real-world approach to low-latency market data processing — all without formal experience in trading environments.  

## Contact

For questions or feedback, you can reach out via my LinkedIn profile: [https://www.linkedin.com/in/roark-m-0a759a175/]


