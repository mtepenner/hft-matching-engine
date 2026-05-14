#include "order_book.hpp"
#include <cstring>
#include <chrono>
#include <stdexcept>

namespace hft {

OrderBook::OrderBook(const char* symbol) {
    std::strncpy(symbol_, symbol, sizeof(symbol_) - 1);
    symbol_[sizeof(symbol_) - 1] = '\0';
}

OrderId OrderBook::nextOrderId() { return nextId_++; }

OrderId OrderBook::addOrder(ClientId clientId, Side side, Price price, Quantity qty,
                              OrderType type, uint64_t timestamp) {
    Order* o = pool_.acquire();
    if (!o) return 0; // pool exhausted

    o->id         = nextOrderId();
    o->clientId   = clientId;
    o->price      = price;
    o->qty        = qty;
    o->filledQty  = 0;
    o->side       = side;
    o->type       = type;
    o->status     = OrderStatus::New;
    o->timestamp  = timestamp;

    // For market orders, set extreme price to guarantee crossing
    if (type == OrderType::Market) {
        o->price = (side == Side::Buy) ? INT64_MAX : INT64_MIN;
    }

    matchOrder(o);

    if (!o->isFilled() && type == OrderType::Limit) {
        // Rest the order in the book
        if (side == Side::Buy) {
            auto& level = bids_[o->price];
            level.price    = o->price;
            level.totalQty += o->remainingQty();
            level.orders.push_back(o);
            auto it = std::prev(level.orders.end());
            orderIndex_[o->id] = std::make_pair(side, it);
        } else {
            auto& level = asks_[o->price];
            level.price    = o->price;
            level.totalQty += o->remainingQty();
            level.orders.push_back(o);
            auto it = std::prev(level.orders.end());
            orderIndex_[o->id] = std::make_pair(side, it);
        }
    } else if (type == OrderType::IOC || type == OrderType::Market) {
        if (!o->isFilled()) {
            o->status = OrderStatus::Cancelled;
        }
        pool_.release(o);
    }

    return o->id;
}

bool OrderBook::cancelOrder(OrderId orderId) {
    auto it = orderIndex_.find(orderId);
    if (it == orderIndex_.end()) return false;

    auto [side, listIt] = it->second;
    Order* o = *listIt;

    if (side == Side::Buy) {
        auto levelIt = bids_.find(o->price);
        if (levelIt == bids_.end()) return false;
        auto& level = levelIt->second;
        level.totalQty -= o->remainingQty();
        level.orders.erase(listIt);
        if (level.orders.empty()) bids_.erase(levelIt);
    } else {
        auto levelIt = asks_.find(o->price);
        if (levelIt == asks_.end()) return false;
        auto& level = levelIt->second;
        level.totalQty -= o->remainingQty();
        level.orders.erase(listIt);
        if (level.orders.empty()) asks_.erase(levelIt);
    }

    orderIndex_.erase(it);
    o->status = OrderStatus::Cancelled;
    pool_.release(o);
    return true;
}

bool OrderBook::modifyOrder(OrderId orderId, Quantity newQty) {
    auto it = orderIndex_.find(orderId);
    if (it == orderIndex_.end()) return false;

    Order* o = *(it->second.second);
    if (newQty >= o->qty) return false; // only reductions without repricing
    Quantity delta = o->qty - newQty;
    o->qty = newQty;

    auto [side, _] = it->second;
    if (side == Side::Buy) {
        bids_[o->price].totalQty -= delta;
    } else {
        asks_[o->price].totalQty -= delta;
    }
    return true;
}

Price OrderBook::bestBid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

Price OrderBook::bestAsk() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

std::vector<OrderBook::L2Level> OrderBook::getL2(int depth) const {
    std::vector<L2Level> levels;
    levels.reserve(depth * 2);

    int n = 0;
    for (auto& [price, lvl] : bids_) {
        if (n++ >= depth) break;
        levels.push_back({price, lvl.totalQty, (int)lvl.orders.size()});
    }
    n = 0;
    for (auto& [price, lvl] : asks_) {
        if (n++ >= depth) break;
        levels.push_back({price, lvl.totalQty, (int)lvl.orders.size()});
    }
    return levels;
}

std::vector<Trade> OrderBook::drainTrades() {
    std::vector<Trade> out;
    out.swap(pendingTrades_);
    return out;
}

void OrderBook::matchOrder(Order* aggressor) {
    if (aggressor->side == Side::Buy) {
        auto& contra = asks_;

        while (!aggressor->isFilled() && !contra.empty()) {
            auto& [topPrice, level] = *contra.begin();

            // Check if prices cross
            bool crosses = aggressor->price >= topPrice;
            if (!crosses) break;

            while (!level.orders.empty() && !aggressor->isFilled()) {
                Order* passive = level.orders.front();
                Quantity fillQty = std::min(aggressor->remainingQty(), passive->remainingQty());

                aggressor->filledQty += fillQty;
                passive->filledQty   += fillQty;
                level.totalQty       -= fillQty;

                Trade t;
                t.aggressorId = aggressor->id;
                t.passiveId   = passive->id;
                t.price       = topPrice;
                t.qty         = fillQty;
                t.timestamp   = aggressor->timestamp;
                pendingTrades_.push_back(t);

                if (passive->isFilled()) {
                    passive->status = OrderStatus::Filled;
                    orderIndex_.erase(passive->id);
                    level.orders.pop_front();
                    pool_.release(passive);
                } else {
                    passive->status = OrderStatus::PartialFill;
                }
            }

            if (level.orders.empty()) {
                contra.erase(contra.begin());
            }
        }
    } else {
        auto& contra = bids_;

        while (!aggressor->isFilled() && !contra.empty()) {
            auto& [topPrice, level] = *contra.begin();

            // Check if prices cross
            bool crosses = aggressor->price <= topPrice;
            if (!crosses) break;

            while (!level.orders.empty() && !aggressor->isFilled()) {
                Order* passive = level.orders.front();
                Quantity fillQty = std::min(aggressor->remainingQty(), passive->remainingQty());

                aggressor->filledQty += fillQty;
                passive->filledQty   += fillQty;
                level.totalQty       -= fillQty;

                Trade t;
                t.aggressorId = aggressor->id;
                t.passiveId   = passive->id;
                t.price       = topPrice;
                t.qty         = fillQty;
                t.timestamp   = aggressor->timestamp;
                pendingTrades_.push_back(t);

                if (passive->isFilled()) {
                    passive->status = OrderStatus::Filled;
                    orderIndex_.erase(passive->id);
                    level.orders.pop_front();
                    pool_.release(passive);
                } else {
                    passive->status = OrderStatus::PartialFill;
                }
            }

            if (level.orders.empty()) {
                contra.erase(contra.begin());
            }
        }
    }

    if (aggressor->isFilled()) {
        aggressor->status = OrderStatus::Filled;
    } else if (aggressor->filledQty > 0) {
        aggressor->status = OrderStatus::PartialFill;
    }
}

} // namespace hft
