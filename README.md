# High-Frequency Trading (HFT) Matching Engine

An ultra-low latency High-Frequency Trading (HFT) matching engine architected for sub-microsecond order execution. This project demonstrates advanced system-level optimizations, including lock-free data structures, memory pooling, and a robust microservices ecosystem for parsing financial protocols and broadcasting market data.

## Table of Contents
- [Features](#features)
- [Architecture](#architecture)
- [Technologies](#technologies)
- [Installation](#installation)
- [License](#license)

## 🚀 Features
- **Sub-Microsecond Execution**: Optimized Price-Time Priority Limit Order Book engineered to match orders in <500ns.
- **Zero-Allocation Data Path**: Utilizes object pools to pre-allocate memory and lock-free ring buffers (LMAX Disruptor pattern) for thread communication.
- **Kernel Bypass Networking**: Advanced UDP networking implementation (compatible with DPDK/Solarflare ef_vi) for minimum network latency.
- **FIX Protocol Support**: A fast binary parser for processing standard Financial Information eXchange (FIX) messages.
- **Real-Time Trading Terminal**: High-performance React UI featuring WebGL-powered order book depth charts and a live ticker tape.
- **Load Testing**: Capable of ingesting and matching 100,000+ orders per second with stable performance.

## 🏗️ Architecture
The system is divided into four highly specialized tiers:
1.  **Exchange Core (C++)**: The foundational matching engine pinned to isolated CPU cores, processing trades with extreme efficiency.
2.  **FIX Gateway (Go)**: Handles TCP connections from clients, parsing FIX messages and pushing them to the C++ core via shared memory.
3.  **Market Data API (Go)**: Subscribes to the engine's output, broadcasting L2/L3 order book updates via WebSockets and aggregating OHLC candlesticks.
4.  **Trading Terminal (React)**: An optimized frontend interface designed to render massive data streams without DOM bottlenecks.

## 🛠️ Technologies
- **Engine Core**: C++ (Lock-free queues, memory pooling, kernel bypass)
- **Gateways & APIs**: Go, WebSockets, FIX Protocol
- **Frontend**: React, TypeScript, WebGL/Canvas
- **Infrastructure**: Docker Compose, CPU Core Isolation Scripts

## 📥 Installation
1. Clone the repository: `git clone https://github.com/mtepenner/hft-matching-engine.git`
2. Prepare the host environment (isolate CPU cores for the engine): `./scripts/core_pinning.sh`
3. Boot the Gateway, API, and UI services: `docker-compose up`
4. Run the C++ Exchange Core natively on your host.

*(Note: To test maximum throughput, execute `./scripts/load_tester.py` to inject 100,000 orders/sec into the system).*

## ⚖️ License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
