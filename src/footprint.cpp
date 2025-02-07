#include "footprint.h"
#include <iostream>
#include "json.hpp"

namespace trading {

using json = nlohmann::json;

std::string FootprintBar::PriceLevel::toJson() const {
    json j;
    j["price"] = std::round(price * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["volume"] = std::round(volume * std::pow(10, volumePrecision)) / std::pow(10, volumePrecision);
    j["bidSize"] = std::round(bidSize * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["askSize"] = std::round(askSize * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["bidCount"] = bidCount;
    j["askCount"] = askCount;
    j["delta"] = std::round(delta * std::pow(10, volumePrecision)) / std::pow(10, volumePrecision);
    j["tradesCount"] = tradesCount;
    return j.dump();
}

FootprintBar::FootprintBar(int duration, int scale, int volumePrecision, int pricePrecision)
    : duration(duration)
    , scale(scale)
    , volumePrecision(volumePrecision)
    , pricePrecision(pricePrecision) {}

double FootprintBar::getPriceLevelHeight() {
    if (priceLevelHeight == 0) {
        priceLevelHeight = scale * std::pow(10, -pricePrecision);
    }
    return priceLevelHeight;
}

double FootprintBar::normalizePrice(double price) {
    double height = getPriceLevelHeight();
    return std::round(std::floor(price / height) * height * std::pow(10, pricePrecision)) /
           std::pow(10, pricePrecision);
}

bool FootprintBar::handleTick(const Trade& tick) {
    if (timestamp == 0) {
        timestamp = tick.time / duration * duration;
        openTime = tick.time;
        closeTime = tick.time;
        open = normalizePrice(tick.price);
        close = normalizePrice(tick.price);
        high = normalizePrice(tick.price);
        low = normalizePrice(tick.price);
    }

    if (tick.time < timestamp || tick.time >= timestamp + duration) {
        return false;
    }

    double tickScalePrice = normalizePrice(tick.price);

    if (priceLevels.find(tickScalePrice) == priceLevels.end()) {
        priceLevels[tickScalePrice] = PriceLevel(volumePrecision, pricePrecision);
        priceLevels[tickScalePrice].price = tickScalePrice;
    }

    auto& priceLevel = priceLevels[tickScalePrice];

    if (tick.time > closeTime) {
        closeTime = tick.time;
        close = tick.price;
    }

    if (tick.time < openTime) {
        openTime = tick.time;
        open = tick.price;
    }

    high = std::max(high, tick.price);
    low = std::min(low, tick.price);

    if (tick.isBuy) {
        priceLevel.askSize += tick.size;
        priceLevel.askCount++;
        priceLevel.delta += tick.size;
    } else {
        priceLevel.bidSize += tick.size;
        priceLevel.bidCount++;
        priceLevel.delta -= tick.size;
    }

    priceLevel.volume += tick.size;
    priceLevel.tradesCount++;

    volume += tick.size;
    tradesCount++;
    delta += tick.isBuy ? tick.size : -tick.size;
    return true;
}

void FootprintBar::fillNoTradesPriceLevels() {
    double priceInterval = std::pow(10, -pricePrecision) * scale;
    double price = normalizePrice(open);
    double priceEnd = normalizePrice(close);

    while (price <= priceEnd) {
        if (priceLevels.find(price) == priceLevels.end()) {
            priceLevels[price] = PriceLevel(volumePrecision, pricePrecision);
            priceLevels[price].price = price;
        }
        price += priceInterval;
    }
}

void FootprintBar::endHandleTick() {
    fillNoTradesPriceLevels();
}

std::string FootprintBar::toJson() const {
    json j;
    j["timestamp"] = timestamp;
    j["duration"] = duration;
    j["scale"] = scale;
    j["openTime"] = openTime;
    j["closeTime"] = closeTime;
    j["open"] = std::round(open * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["high"] = std::round(high * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["low"] = std::round(low * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["close"] = std::round(close * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision);
    j["volume"] = std::round(volume * std::pow(10, volumePrecision)) / std::pow(10, volumePrecision);
    j["delta"] = std::round(delta * std::pow(10, volumePrecision)) / std::pow(10, volumePrecision);
    j["tradesCount"] = tradesCount;
    j["volumePrecision"] = volumePrecision;
    j["pricePrecision"] = pricePrecision;

    json priceLevelsJson;
    for (const auto& [price, level] : priceLevels) {
        std::string priceStr = std::to_string(std::round(price * std::pow(10, pricePrecision)) / std::pow(10, pricePrecision));
        priceLevelsJson[priceStr] = json::parse(level.toJson());
    }
    j["priceLevels"] = priceLevelsJson;

    return j.dump(4);
}

} // namespace trading 