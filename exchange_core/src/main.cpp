#include "engine/order_book.hpp"
#include "engine/matcher.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>

#ifdef __linux__
#include <sched.h>
#include <pthread.h>
#include <unistd.h>

static bool pinToCore(int core) {
    if (core < 0) return false;
    long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpuCount <= 0 || core >= cpuCount) return false;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}
#else
static bool pinToCore(int) { return false; }
#endif

int main(int argc, char* argv[]) {
    int coreId = 2; // default to core 2 (0,1 often reserved for OS)
    if (argc > 1) coreId = std::atoi(argv[1]);

    if (pinToCore(coreId)) {
        std::cout << "[ExchangeCore] Pinned to CPU core " << coreId << "\n";
    } else {
        std::cout << "[ExchangeCore] CPU pinning unavailable for core "
                  << coreId << ", continuing without affinity\n";
    }

    std::vector<std::string> symbols = {"AAPL", "MSFT", "TSLA", "SPY", "QQQ"};
    hft::Matcher matcher(symbols);

    std::cout << "[ExchangeCore] Matching engine online – " << symbols.size()
              << " symbols\n";

    // Simple benchmark: inject N orders to validate matching throughput
    const int N = 100000;
    auto start = std::chrono::high_resolution_clock::now();

    uint64_t ts = 1000000000ULL;
    for (int i = 0; i < N; ++i) {
        hft::Price price = 15000 + (i % 100) * hft::PRICE_TICK; // $150.00 ± $1
        hft::Side  side  = (i % 2 == 0) ? hft::Side::Buy : hft::Side::Sell;
        matcher.submitOrder("AAPL", static_cast<hft::ClientId>(i % 100),
                            side, price, 100, hft::OrderType::Limit, ts + i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double ops = N / ms * 1000.0;

    std::cout << "[ExchangeCore] " << N << " orders in " << ms << " ms  ("
              << static_cast<uint64_t>(ops) << " orders/sec)\n";

    auto trades = matcher.pollTrades("AAPL");
    std::cout << "[ExchangeCore] " << trades.size() << " trades executed\n";

    return 0;
}
