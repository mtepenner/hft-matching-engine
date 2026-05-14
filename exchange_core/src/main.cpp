#include "engine/order_book.hpp"
#include "engine/matcher.cpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>

#ifdef __linux__
#include <sched.h>
#include <pthread.h>

static void pinToCore(int core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
#else
static void pinToCore(int) {} // no-op on non-Linux
#endif

int main(int argc, char* argv[]) {
    int coreId = 2; // default to core 2 (0,1 often reserved for OS)
    if (argc > 1) coreId = std::atoi(argv[1]);

    pinToCore(coreId);
    std::cout << "[ExchangeCore] Pinned to CPU core " << coreId << "\n";

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
