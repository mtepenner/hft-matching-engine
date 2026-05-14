#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include "../../include/types.hpp"
#include "../memory/object_pool.hpp"

namespace hft {

// Price level: a list of resting orders at a given price.
struct PriceLevel {
    Price             price;
    std::list<Order*> orders;
    Quantity          totalQty = 0;
};

/**
 * Price-Time Priority Limit Order Book.
 * Bids sorted descending, asks ascending.
 */
class OrderBook {
public:
    explicit OrderBook(const char* symbol);

    // Submit a new order. Returns the order ID assigned.
    OrderId addOrder(ClientId clientId, Side side, Price price, Quantity qty,
                     OrderType type, uint64_t timestamp);

    // Cancel a resting order. Returns false if not found.
    bool cancelOrder(OrderId orderId);

    // Modify quantity (only reduction allowed without repricing).
    bool modifyOrder(OrderId orderId, Quantity newQty);

    // Best bid/ask prices (0 if empty).
    Price bestBid() const;
    Price bestAsk() const;

    // L2 snapshot: top N levels.
    struct L2Level { Price price; Quantity qty; int orderCount; };
    std::vector<L2Level> getL2(int depth = 10) const;

    // All trades executed since last call (drains the trade buffer).
    std::vector<Trade> drainTrades();

    const char* symbol() const { return symbol_; }

private:
    void matchOrder(Order* aggressor);
    OrderId nextOrderId();

    char symbol_[16];
    uint64_t nextId_ = 1;

    // Bids: highest price first
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // Asks: lowest price first
    std::map<Price, PriceLevel, std::less<Price>>    asks_;

    // Fast lookup by order id
    std::unordered_map<OrderId, std::pair<Side, std::list<Order*>::iterator>> orderIndex_;

    // Pool for order objects
    static constexpr std::size_t POOL_SIZE = 1 << 16; // 65536
    ObjectPool<Order, POOL_SIZE> pool_;

    std::vector<Trade> pendingTrades_;
};

} // namespace hft
