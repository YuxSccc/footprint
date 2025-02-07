#pragma once
#include "config.h"
#include "data_fetcher.h"
#include "output_handler.h"
#include "footprint.h"
#include <string>
#include <chrono>
#include <memory>

namespace trading {

struct ProcessingStats {
    size_t totalTrades{0};
    size_t aggregatedTrades{0};
    std::chrono::milliseconds parseTime{0};
    std::chrono::milliseconds writeTime{0};
    
    void print(const std::string& filename) const;
};

struct AggTrade {
    int64_t id;
    double price;
    double qty;
    double quoteQty;
    int64_t time;
    bool isBuyerMaker;
    int count;
};

class Processor {
public:
    Processor(std::unique_ptr<DataFetcher> fetcher,
             std::unique_ptr<OutputHandler> outputHandler,
             const ProcessConfig& processConfig,
             const SymbolConfig& symbolConfig);
             
    void processFile(const std::string& filename);
    
private:
    std::unique_ptr<DataFetcher> fetcher_;
    std::unique_ptr<OutputHandler> outputHandler_;
    ProcessConfig processConfig_;
    SymbolConfig symbolConfig_;
    
    std::vector<Trade> parseFile(const std::string& filename, ProcessingStats& stats);
    std::vector<FootprintBar> generateFootprint(const std::vector<Trade>& trades);
    std::vector<AggTrade> generateAggTrades(const std::vector<Trade>& trades);
    void writeAggTrades(const std::string& filename, const std::vector<AggTrade>& aggTrades);
};

} // namespace trading 