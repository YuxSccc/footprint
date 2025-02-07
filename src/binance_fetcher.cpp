#include "data_fetcher.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace trading {

namespace {
constexpr double EPSILON = 1e-9;
}

std::unique_ptr<DataFetcher> DataFetcher::create(const std::string& exchange) {
    if (exchange == "binance") {
        return std::make_unique<BinanceFetcher>();
    }
    throw std::runtime_error("Unsupported exchange: " + exchange);
}

int64_t BinanceFetcher::fastAtoll(const char* str, const char** end) {
    int64_t val = 0;
    bool neg = false;
    if (*str == '-') {
        neg = true;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (*str - '0');
        str++;
    }
    *end = str;
    return neg ? -val : val;
}

double BinanceFetcher::fastAtof(const char* str, const char** end) {
    double val = 0;
    bool neg = false;
    if (*str == '-') {
        neg = true;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (*str - '0');
        str++;
    }

    if (*str == '.') {
        str++;
        double factor = 0.1;
        while (*str >= '0' && *str <= '9') {
            val += (*str - '0') * factor;
            factor *= 0.1;
            str++;
        }
    }
    val += EPSILON;
    *end = str;
    return neg ? -val : val;
}

Trade BinanceFetcher::parseLine(const char* start, const char** end) {
    Trade trade;
    const char* p = start;

    trade.id = fastAtoll(p, &p);
    p++; // skip comma

    trade.price = fastAtof(p, &p);
    p++; // skip comma

    trade.qty = fastAtof(p, &p);
    p++; // skip comma

    trade.quoteQty = fastAtof(p, &p);
    p++; // skip comma

    trade.time = fastAtoll(p, &p);
    p++; // skip comma

    trade.isBuyerMaker = (p[0] == 't' || p[0] == 'T' || p[0] == '1');
    trade.isBuy = !trade.isBuyerMaker;
    trade.size = trade.qty;

    // 跳过到行尾
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    *end = p;
    return trade;
}

bool BinanceFetcher::isHeader(const char* line) {
    return strstr(line, "id,price") != nullptr;
}

} // namespace trading 