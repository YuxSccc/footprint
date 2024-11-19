#include "footprint.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <semaphore>
#include <thread>

namespace fs = std::filesystem;
using json = nlohmann::json;
double eps = 1e-9;
constexpr int BTC_DURATION = 60 * 5;
constexpr int BTC_SCALE = 100;
constexpr int BTC_VOLUME_PRECISION = 2;
constexpr int BTC_PRICE_PRECISION = 1;

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
        : duration(duration), scale(scale),
          volumePrecision(volumePrecision), pricePrecision(pricePrecision) {}

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
    // Map is already sorted in C++
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
        priceLevelsJson[std::to_string(price)] = json::parse(level.toJson());
    }
    j["priceLevels"] = priceLevelsJson;

    return j.dump(4);
}

bool hasHeader(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) return false;

    std::string firstLine;
    std::getline(file, firstLine);
    return firstLine.find("id,price") != std::string::npos;
}

inline int64_t fast_atoll(const char *str, char const **end)
{
    int64_t val = 0;
    bool neg = false;
    if (*str == '-')
    {
        neg = true;
        str++;
    }
    while (*str >= '0' && *str <= '9')
    {
        val = val * 10 + (*str - '0');
        str++;
    }
    *end = str;
    return neg ? -val : val;
}

inline double fast_atof(const char *str, char const **end)
{
    double val = 0;
    bool neg = false;
    if (*str == '-')
    {
        neg = true;
        str++;
    }

    while (*str >= '0' && *str <= '9')
    {
        val = val * 10 + (*str - '0');
        str++;
    }

    if (*str == '.')
    {
        str++;
        double factor = 0.1;
        while (*str >= '0' && *str <= '9')
        {
            val += (*str - '0') * factor;
            factor *= 0.1;
            str++;
        }
    }
    val += eps;
    *end = str;
    return neg ? -val : val;
}

Trade parse_line(const char *start, const char **end)
{
    Trade trade;
    const char *p = start;

    trade.id = fast_atoll(p, &p);
    p++;

    trade.price = fast_atof(p, &p);
    p++;

    trade.qty = fast_atof(p, &p);
    p++;

    trade.quote_qty = fast_atof(p, &p);
    p++;

    trade.time = fast_atoll(p, &p);
    p++;

    trade.is_buyer_maker = (p[0] == 't' || p[0] == 'T' || p[0] == '1');
    while (*p && *p != '\n')
        p++;
    if (*p == '\n')
        p++;

    *end = p;
    trade.isBuy = !trade.is_buyer_maker;
    trade.size = trade.qty;
    return trade;
}

bool processFile(const std::string& filename, const std::string& inputDir, const std::string& outputDir) {
    try {
        std::string inputPath = inputDir + filename;
        fs::path outputPath = outputDir + fs::path(filename).stem().string() + ".json";

        if (fs::exists(outputPath)) {
            std::cout << "Skip existing file: " << filename << std::endl;
            return true;
        }

        std::cout << "Processing: " << filename << std::endl;
        auto startTime = std::chrono::steady_clock::now();

        std::map<int64_t, std::unique_ptr<FootprintBar>> footprintList;
        auto* currentFootprint = new FootprintBar(BTC_DURATION, BTC_SCALE, BTC_VOLUME_PRECISION, BTC_PRICE_PRECISION);

        std::string line;
        size_t lineCount = 0;
        auto lastPrintTime = std::chrono::steady_clock::now();

        std::ifstream infile(inputPath, std::ios::binary);
        if (!infile)
        {
            throw std::runtime_error("Failed to open file");
        }

        const size_t BUFFER_SIZE = 512 * 1024 * 1024;
        std::vector<char> buffer(BUFFER_SIZE);
        std::string content;
        content.reserve(BUFFER_SIZE * 2);
        bool eof = false;
        const size_t MIN_REMAINING = 10000;

        auto parse_start = std::chrono::high_resolution_clock::now();

        if (infile.read(buffer.data(), BUFFER_SIZE) || infile.gcount() > 0) {
            size_t read_size = infile.gcount();
            content.append(buffer.data(), read_size);

            const char *p = content.data();
            const char *end = p + content.size();

            if (strstr(p, "id,price") != nullptr) {
                while (p < end && *p && *p != '\n')
                    p++;
                if (p < end && *p == '\n')
                    p++;
            }

            while (true) {
                if (p >= end) {
                    break;
                }

                size_t remaining = end - p;
                if (!eof && remaining < MIN_REMAINING) {
                    std::string new_content;
                    new_content.reserve(BUFFER_SIZE * 2);

                    if (remaining > 0) {
                        new_content.assign(p, remaining);
                    }

                    if (infile.read(buffer.data(), BUFFER_SIZE) || infile.gcount() > 0) {
                        size_t read_size = infile.gcount();
                        new_content.append(buffer.data(), read_size);

                        if (read_size < BUFFER_SIZE) {
                            eof = true;
                        }
                    } else {
                        eof = true;
                    }

                    content = std::move(new_content);

                    // 更新指针
                    p = content.data();
                    end = p + content.size();

                    // 如果没有数据了，退出循环
                    if (content.empty()) {
                        break;
                    }
                }

                // 确保有足够的数据来解析一行
                if (p >= end) {
                    break;
                }

                // 处理一条交易记录
                const char *next_p = p;
                Trade trade = parse_line(p, &next_p);
                trade.time = trade.time / 1000;
                // 确保指针确实前进了
                if (next_p <= p || next_p > end) {
                    break;  // 防止无限循环或越界
                }
                p = next_p;
                if (currentFootprint->handleTick(trade)) {
                    continue;
                }

                // 当前K线结束，保存并创建新的
                if (currentFootprint->timestamp != 0) {
                    footprintList[currentFootprint->timestamp] =
                            std::unique_ptr<FootprintBar>(currentFootprint);
                }

                currentFootprint = new FootprintBar(
                        BTC_DURATION, BTC_SCALE, BTC_VOLUME_PRECISION, BTC_PRICE_PRECISION);
                if (!currentFootprint->handleTick(trade)) {
                    std::cerr << "Handle tick failed for " << filename << std::endl;
                    delete currentFootprint;
                    return false;
                }
            }
        }

        // 处理最后一个K线
        if (currentFootprint->timestamp != 0) {
            footprintList[currentFootprint->timestamp] =
                    std::unique_ptr<FootprintBar>(currentFootprint);
        } else {
            delete currentFootprint;
        }

        // 结束处理所有K线
        for (auto& [_, footprint] : footprintList) {
            footprint->endHandleTick();
        }

        // 确保输出目录存在
        fs::create_directories("./footprint");

        // 写入JSON文件
        std::ofstream outFile(outputPath);
        if (!outFile) {
            std::cerr << "Failed to open output file: " << outputPath << std::endl;
            return false;
        }

        // 构建JSON对象
        json outputJson;
        for (const auto& [timestamp, footprint] : footprintList) {
            outputJson[std::to_string(timestamp)] = json::parse(footprint->toJson());
        }

        outFile << outputJson.dump(4);
        outFile.flush();
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        std::cout << "Completed " << filename << " in " << duration.count() << " seconds" << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing " << filename << ": " << e.what() << std::endl;
        return false;
    }
}

int main() {
    const std::string input_dir = R"(E:\orderdata\binance\rawdata\)";
    const std::string output_dir = R"(E:\orderdata\binance\footprint\)";
    constexpr int workerCount = 6;
    std::counting_semaphore<workerCount> sem(workerCount);
    try {
        // 创建输出目录
        fs::create_directories(output_dir);

        // 获取所有需要处理的文件
        std::vector<std::string> files;
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            if (entry.path().extension() == ".csv") {
                files.push_back(entry.path().filename().string());
            }
        }

        std::cout << "Found " << files.size() << " files to process" << std::endl;

        size_t successful = 0;
        size_t failed = 0;
        auto startTime = std::chrono::steady_clock::now();
        std::vector<std::thread> workerList;
        // 处理所有文件
        for (size_t i = 0; i < files.size(); ++i) {
            sem.acquire();
            auto task = [&, length=files.size()](std::string file, int i) {
                if (processFile(file, input_dir, output_dir)) {
                    successful++;
                } else {
                    failed++;
                }
                std::cout << "Progress: " << (i + 1) << "/" << length << " files completed" << std::endl;
                sem.release();
            };
            workerList.push_back(std::move(std::thread(task, files[i], i)));
        }
        for (auto& thread: workerList) {
            thread.join();
        }

        // 输出统计信息
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

        std::cout << "\nProcessing completed:\n"
                  << "Total time: " << duration.count() << " seconds\n"
                  << "Total files: " << files.size() << "\n"
                  << "Successful: " << successful << "\n"
                  << "Failed: " << failed << "\n"
                  << "Average time per file: "
                  << (duration.count() / static_cast<double>(files.size())) << " seconds\n";

        return (failed == 0) ? 0 : 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}