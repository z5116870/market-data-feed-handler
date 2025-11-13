# market-data-feed-handler
This project is a C++ application designed to parse exchange market feeds in real time via UDP multicast traffic. To be used in conjunction with [itch-message-generator](https://github.com/z5116870/itch-message-generator/) which runs parallel to this and provides the market feed. Subscription to real ITCH feeds from real exchanges isn't possible for me :(

## Key Features

- **Real-time Data Handling**: Continuously listens to UDP multicast feeds, capable of processing high-throughput data with minimal latency.
- **Reliable Parsing**: Converts raw exchange messages into structured data formats for order book updates, trades, and market events.
- **Modular Design**: Parsing logic and network handling are separated for maintainability and easy extension.
- **Testing and Validation**: Includes test cases to verify parsing accuracy and robustness against malformed or out-of-order messages.
- **Cross-Platform**: Built in C++ with standard networking APIs, compatible with Linux and Windows environments.

## Showcase / Program Capabilities

1. **Connecting to Live Market Feeds**
   - Joins UDP multicast groups to receive real-time data.
   - Handles multiple feeds simultaneously.
   - Automatically recovers from temporary packet loss or network interruptions.

2. **Parsing and Structuring Market Data**
   - Processes raw exchange messages into structured formats.
   - Supports order book updates, trades, and other market events.
   - Maintains high-performance parsing suitable for low-latency applications.

3. **Extensible and Testable**
   - Helper functions and parsing modules are separated for clarity.
   - Unit tests verify correctness against sample and edge-case messages.
   - Easily extendable to support additional exchanges or message types.

4. **Performance-Oriented Design**
   - Minimal memory overhead and efficient handling of high message volumes.
   - Optimized for low-latency applications such as algorithmic trading systems.

## Example Use Case

A typical workflow might include:  

1. Starting the program and connecting to a multicast feed.
2. Receiving and parsing thousands of messages per second.
3. Using the structured output to update a real-time order book or feed a trading algorithm.

## Technical Highlights

- Written entirely in C++ for performance.
- Uses standard socket programming for UDP multicast handling.
- Modular, maintainable codebase with separate parsing and utility modules.
- Includes test scripts and sample data for validation.

## Project Structure
```
market-data-feed-handler/
├─ main.cpp # Entry point: initializes network and starts feed handling
├─ parse.cpp # Parsing logic: raw messages → structured data
├─ parse.h # Header for parsing module
├─ helper.h # Utility functions
├─ mdfh/ # Additional source/support files
├─ test/ # Test cases and sample data
├─ screenshots/ # Example outputs or logs
├─ .gitignore
└─ README.md
```

## Why This Project Matters
Market data feeds are the backbone of modern trading systems. This program demonstrates the ability to:

- Work with **real-time, high-volume data streams**.
- Write **efficient, maintainable C++ code**.
- Build **robust, testable systems** that could integrate with trading or analytics platforms.
- Apply **networking and low-latency programming skills**, which are critical in finance and other real-time domains.

