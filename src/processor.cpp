#include "processor.h"
#include "io_utils.h"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>

namespace trading {

namespace fs = std::filesystem;

void ProcessingStats::print(const std::string& filename) const {
    std::cout << "\nCompleted processing " << filename << "\n"
              << "Total trades: " << totalTrades << "\n"
              << "Aggregated trades: " << aggregatedTrades << "\n"
              << "Parse time: " << parseTime.count() << "ms\n"
              << "Write time: " << writeTime.count() << "ms\n"
              << "Total time: " << (parseTime + writeTime).count() << "ms\n"
              << std::endl;
}

Processor::Processor(
    std::unique_ptr<DataFetcher> fetcher,
    std::unique_ptr<OutputHandler> outputHandler,
    const ProcessConfig& processConfig,
    const SymbolConfig& symbolConfig)
    : fetcher_(std::move(fetcher))
    , outputHandler_(std::move(outputHandler))
    , processConfig_(processConfig)
    , symbolConfig_(symbolConfig) {}

void Processor::processFile(const std::string& filename) {
    fs::path footprintPath = fs::path(processConfig_.outputDir) / "footprint" / 
                            fs::path(filename).filename().replace_extension(".json");
    fs::path aggTradePath = fs::path(processConfig_.outputDir) / "aggtrade" / 
                           fs::path(filename).filename();
    
    // 检查输出文件是否已存在
    if (fs::exists(footprintPath) && fs::exists(aggTradePath)) {
        std::cout << "Skip existing file: " << filename << std::endl;
        return;
    }

    std::cout << "Processing: " << filename << std::endl;
    ProcessingStats stats;
    
    try {
        auto parseStart = std::chrono::high_resolution_clock::now();
        auto trades = parseFile(filename, stats);
        auto parseEnd = std::chrono::high_resolution_clock::now();
        
        stats.parseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            parseEnd - parseStart);
        
        auto writeStart = std::chrono::high_resolution_clock::now();
        
        // 生成并写入footprint
        if (!fs::exists(footprintPath)) {
            auto footprints = generateFootprint(trades);
            fs::create_directories(footprintPath.parent_path());
            outputHandler_->write(footprintPath.string(), footprints, symbolConfig_);
        }
        
        // 生成并写入aggtrade
        if (!fs::exists(aggTradePath)) {
            auto aggTrades = generateAggTrades(trades);
            fs::create_directories(aggTradePath.parent_path());
            writeAggTrades(aggTradePath.string(), aggTrades);
        }
        
        auto writeEnd = std::chrono::high_resolution_clock::now();
        stats.writeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            writeEnd - writeStart);
        
        stats.print(filename);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing " << filename << ": " << e.what() << std::endl;
    }
}

std::vector<Trade> Processor::parseFile(
    const std::string& filename, 
    ProcessingStats& stats) {
    
    std::vector<Trade> trades;
    io::FileReader reader(filename);
    
    // 先读取第一个chunk
    if (!reader.readChunk()) {
        return trades;
    }
    
    // 处理header
    if (fetcher_->isHeader(reader.current())) {
        const char* p = reader.current();
        const char* end = reader.end();
        size_t offset = 0;
        while (p < end && *p && *p != '\n') {
            p++;
            offset++;
        }
        if (p < end && *p == '\n') {
            p++;
            offset++;
        }
        reader.advance(offset);
    }

    // 主处理循环
    while (true) {
        if (reader.current() >= reader.end()) {
            if (reader.eof()) break;
            if (!reader.readChunk()) break;
        }
        
        size_t remaining = reader.end() - reader.current();
        if (remaining < io::FileReader::MIN_REMAINING && !reader.eof()) {
            if (!reader.readChunk()) break;
        }
        
        const char* next_p = reader.current();
        Trade trade = fetcher_->parseLine(reader.current(), &next_p);
        
        if (next_p <= reader.current() || next_p > reader.end()) break;
        
        reader.advance(next_p - reader.current());
        
        trade.time /= 1000; // 转换为秒
        trades.push_back(trade);
        stats.totalTrades++;
    }
    
    // 对trades按时间和ID排序
    std::sort(trades.begin(), trades.end());
    
    return trades;
}

std::vector<FootprintBar> Processor::generateFootprint(
    const std::vector<Trade>& trades) {
    
    std::vector<FootprintBar> footprintList;
    if (trades.empty()) {
        return footprintList;
    }

    auto currentBar = std::make_unique<FootprintBar>(
        symbolConfig_.duration,
        symbolConfig_.scale,
        symbolConfig_.volumePrecision,
        symbolConfig_.pricePrecision
    );

    for (const auto& trade : trades) {
        if (!currentBar->handleTick(trade)) {
            // 当前K线结束，保存并创建新的
            currentBar->endHandleTick();
            footprintList.push_back(*currentBar);
            
            // 创建新的K线
            currentBar = std::make_unique<FootprintBar>(
                symbolConfig_.duration,
                symbolConfig_.scale,
                symbolConfig_.volumePrecision,
                symbolConfig_.pricePrecision
            );
            
            if (!currentBar->handleTick(trade)) {
                std::cerr << "Failed to handle trade in new bar" << std::endl;
                continue;
            }
        }
    }

    // 处理最后一个K线
    if (currentBar->timestamp != 0) {
        currentBar->endHandleTick();
        footprintList.push_back(*currentBar);
    }

    return footprintList;
}

std::vector<AggTrade> Processor::generateAggTrades(const std::vector<Trade>& trades) {
    std::vector<AggTrade> aggregated;
    if (trades.empty()) {
        return aggregated;
    }

    AggTrade* buySideAgg = nullptr;
    AggTrade* sellSideAgg = nullptr;
    std::unique_ptr<AggTrade> buySideHolder;
    std::unique_ptr<AggTrade> sellSideHolder;

    for (const auto& trade : trades) {
        // 计算时间戳
        int64_t ts = (trade.time * 1000 / symbolConfig_.preAggDuration) * symbolConfig_.preAggDuration;

        AggTrade** opAgg = trade.isBuyerMaker ? &buySideAgg : &sellSideAgg;
        std::unique_ptr<AggTrade>& holder = trade.isBuyerMaker ? buySideHolder : sellSideHolder;

        // 如果存在聚合记录但时间不同，保存并重置
        if (*opAgg && (*opAgg)->time != ts) {
            aggregated.push_back(**opAgg);
            *opAgg = nullptr;
            holder.reset();
        }

        // 创建新的聚合记录或更新现有记录
        if (*opAgg == nullptr) {
            holder = std::make_unique<AggTrade>();
            *opAgg = holder.get();
            (*opAgg)->id = trade.id;
            (*opAgg)->price = trade.price;
            (*opAgg)->qty = trade.qty;
            (*opAgg)->quoteQty = trade.quoteQty;
            (*opAgg)->time = ts;
            (*opAgg)->isBuyerMaker = trade.isBuyerMaker;
            (*opAgg)->count = 1;
        } else {
            (*opAgg)->qty += trade.qty;
            (*opAgg)->quoteQty += trade.quoteQty;
            (*opAgg)->count++;
        }
    }

    // 添加最后的聚合记录
    if (buySideAgg) {
        aggregated.push_back(*buySideAgg);
    }
    if (sellSideAgg) {
        aggregated.push_back(*sellSideAgg);
    }

    // 按时间和ID排序
    std::sort(aggregated.begin(), aggregated.end(),
              [](const AggTrade& a, const AggTrade& b) {
                  if (a.time == b.time)
                      return a.id < b.id;
                  return a.time < b.time;
              });

    return aggregated;
}

void Processor::writeAggTrades(const std::string& filename, const std::vector<AggTrade>& aggTrades) {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }

    // 设置8MB的缓冲区
    const size_t BUFFER_SIZE = 8 * 1024 * 1024;
    std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
    outfile.rdbuf()->pubsetbuf(buffer.get(), BUFFER_SIZE);

    // 写入CSV头
    outfile << "id,price,qty,quote_qty,time,is_buyer_maker,count\n";

    // 为每行预分配内存
    std::string line;
    line.reserve(128);  // 预估每行的最大长度

    // 快速整数和浮点数格式化
    char numBuf[32];
    
    for (const auto& trade : aggTrades) {
        line.clear();
        
        // id
        auto p = io::FastFormatter::formatInt(numBuf, trade.id);
        line.append(numBuf, p - numBuf);
        line += ',';
        
        // price
        p = io::FastFormatter::formatDouble(numBuf, trade.price, symbolConfig_.pricePrecision);
        line.append(numBuf, p - numBuf);
        line += ',';
        
        // qty
        p = io::FastFormatter::formatDouble(numBuf, trade.qty, symbolConfig_.volumePrecision);
        line.append(numBuf, p - numBuf);
        line += ',';
        
        // quote_qty
        p = io::FastFormatter::formatDouble(numBuf, trade.quoteQty, symbolConfig_.volumePrecision);
        line.append(numBuf, p - numBuf);
        line += ',';
        
        // time
        p = io::FastFormatter::formatInt(numBuf, trade.time);
        line.append(numBuf, p - numBuf);
        line += ',';
        
        // is_buyer_maker
        line += trade.isBuyerMaker ? '1' : '0';
        line += ',';
        
        // count
        p = io::FastFormatter::formatInt(numBuf, trade.count);
        line.append(numBuf, p - numBuf);
        line += '\n';
        
        outfile.write(line.data(), line.size());
    }
    
    outfile.flush();
}

} // namespace trading 