#pragma once
#include "trade.h"
#include "json.hpp"
#include <map>
#include <string>
#include <cmath>

namespace trading {

class FootprintBar {
public:
    struct PriceLevel {
        double price{0.0};
        double volume{0.0};
        double bidSize{0.0};
        double askSize{0.0};
        int bidCount{0};
        int askCount{0};
        double delta{0.0};
        int tradesCount{0};
        int volumePrecision{0};
        int pricePrecision{0};

        PriceLevel(int vp = 0, int pp = 0)
            : volumePrecision(vp), pricePrecision(pp) {}

        std::string toJson() const;
    };

    FootprintBar(int duration = 0, int scale = 0,
                int volumePrecision = 0, int pricePrecision = 0);

    bool handleTick(const Trade& tick);
    void endHandleTick();
    std::string toJson() const;

    int64_t timestamp{0};
    int duration{0};
    int scale{0};
    std::map<double, PriceLevel> priceLevels;

    int64_t openTime{0};
    int64_t closeTime{0};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    double delta{0.0};
    int tradesCount{0};
    int volumePrecision{0};
    int pricePrecision{0};

private:
    double priceLevelHeight{0.0};

    double getPriceLevelHeight();
    double normalizePrice(double price);
    void fillNoTradesPriceLevels();
};

} // namespace trading 