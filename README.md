# market-data-feed-handler
This project is a C++ application designed to parse exchange market feeds in real time via UDP multicast traffic. To be used in conjunction with [itch-message-generator](https://github.com/z5116870/itch-message-generator/) which runs parallel to this and provides the market feed. Subscription to real ITCH feeds from real exchanges isn't possible for me :(

## Build
To run the program navigate to `build/` and follow the instructions. Make sure you run itch-message-generator first using a certain `MULTICAST_IP` and `PORT`, then run MDFH using:
```bash
sudo ./build/bin/mdfh <MULTICAST_IP> <PORT>
```
### 1. Configure the Build

Run:
```
cmake ..
```

This generates Makefiles and configures the build system.

### 2. Build Everything

Run:
```
make -j
```

This produces the following executables:
```
bin/mdfh
bin/mdfh_unit_tests
bin/benchmark_multithreading
```

All executables are located in:
```
build/bin/
```

### 3. Running the Main Application

Run:
```
sudo ./bin/mdfh <MULTICAST_IP> <PORT>
```

### 4. Running Tests
Option A: Run the test binary directly
```
./bin/mdfh_unit_tests
```

To filter tests:
```
./bin/mdfh_unit_tests --gtest_filter=ParseTests.*
```

Option B: Run tests via CTest
```
ctest -V
```

Very verbose mode:
```
ctest -VV
```
### 5. Running Benchmarks

Run:
```
./bin/benchmark_multithreading
```

### 6. Reconfiguring or Rebuilding

If CMake files changed:
```
cmake ..
make -j
```

If only source files changed:
```
make -j
```

### 7. Cleaning the Build Folder

To remove all build artifacts:
```
rm -rf *
```
Then regenerate:
```
cmake ..
make -j
```

### 8. Directory Layout (Inside build/)

```
build/
├── bin/ (Executables)
├── lib/ (Libraries if generated)
├── CTestTestfile.cmake (CTest registry)
├── CMakeCache.txt
├── Makefile
└── CMakeFiles/ (Auto-generated CMake metadata)
```

### Summary

- Run `sudo ./bin/mdfh <MULTICAST_IP> <PORT>`
- Run `cmake ..` and `make -j` inside build/.
- Binaries appear in `build/bin/.`
- Run tests using `ctest -V` or `./bin/mdfh_unit_tests`
- Run benchmarks using `./bin/benchmark_multithreading`.

## Directory Layout

```
market-data-feed-handler/
├── build/          # Build directory (binaries in bin/)
├── docs/           # All docs and diagrams
├── include/        # Public headers (including inline functions)
├── screenshots/    # Example outputs, logs, benchmark results or diagrams
├── src/            # Source code (parsing logic, main, etc.)
├── test/           # Tests and sample data for parsing / packet handling
├── .gitignore
├── CMakeLists.txt
├── LICENSE
└── README.md
```

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


