#pragma once
#include <cstdint>
#include <cstring>

namespace hft {

// Side of the order
enum class Side : uint8_t { Buy = 0, Sell = 1 };

// Type of order
enum class OrderType : uint8_t { Limit = 0, Market = 1, IOC = 2, FOK = 3 };

// Order status
enum class OrderStatus : uint8_t { New, PartialFill, Filled, Cancelled, Rejected };

using OrderId  = uint64_t;
using Price    = int64_t;   // price in ticks (avoids float)
using Quantity = uint32_t;
using ClientId = uint32_t;

constexpr Price PRICE_TICK = 100; // 1 cent = 100 units

struct alignas(64) Order {
    OrderId   id;
    ClientId  clientId;
    Price     price;
    Quantity  qty;
    Quantity  filledQty;
    Side      side;
    OrderType type;
    OrderStatus status;
    uint64_t  timestamp;    // nanoseconds since epoch

    Quantity remainingQty() const { return qty - filledQty; }
    bool isFilled() const { return filledQty >= qty; }
};

struct Trade {
    OrderId  aggressorId;
    OrderId  passiveId;
    Price    price;
    Quantity qty;
    uint64_t timestamp;
};

} // namespace hft
