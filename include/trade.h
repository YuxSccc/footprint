#pragma once
#include <cstdint>

namespace trading {

struct Trade {
    int64_t id;
    double price;
    double qty;
    double quoteQty;
    int64_t time;
    bool isBuyerMaker;
    bool isBuy;
    double size;

    bool operator<(const Trade& other) const {
        if (time == other.time) return id < other.id;
        return time < other.time;
    }
};

} // namespace trading 