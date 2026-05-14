#pragma once
#include "order_book.hpp"
#include <iostream>
#include <vector>

namespace hft {

class Matcher {
public:
    explicit Matcher(const std::vector<std::string>& symbols) {
        for (const auto& sym : symbols) {
            books_.emplace_back(sym.c_str());
        }
    }

    OrderId submitOrder(const char* symbol, ClientId clientId, Side side,
                        Price price, Quantity qty, OrderType type,
                        uint64_t timestamp) {
        OrderBook* book = findBook(symbol);
        if (!book) return 0;
        return book->addOrder(clientId, side, price, qty, type, timestamp);
    }

    bool cancel(const char* symbol, OrderId orderId) {
        OrderBook* book = findBook(symbol);
        if (!book) return false;
        return book->cancelOrder(orderId);
    }

    std::vector<Trade> pollTrades(const char* symbol) {
        OrderBook* book = findBook(symbol);
        if (!book) return {};
        return book->drainTrades();
    }

    Price bestBid(const char* symbol) {
        OrderBook* book = findBook(symbol);
        return book ? book->bestBid() : 0;
    }

    Price bestAsk(const char* symbol) {
        OrderBook* book = findBook(symbol);
        return book ? book->bestAsk() : 0;
    }

private:
    OrderBook* findBook(const char* symbol) {
        for (auto& book : books_) {
            if (std::strcmp(book.symbol(), symbol) == 0) return &book;
        }
        return nullptr;
    }

    std::vector<OrderBook> books_;
};

} // namespace hft
