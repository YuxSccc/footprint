#pragma once
#include <string>
#include <cstdint>

namespace trading {

struct SymbolConfig {
    int64_t duration;         // Bar duration in seconds
    int scale;               // Price scale
    int volumePrecision;     // Volume precision
    int pricePrecision;      // Price precision
    int64_t preAggDuration;  // Pre-aggregation duration in milliseconds
    
    // Default constructor with BTC-specific values
    SymbolConfig() 
        : duration(300)        // 5 minutes
        , scale(100)
        , volumePrecision(2)
        , pricePrecision(1)
        , preAggDuration(100)  // 100ms for pre-aggregation
    {}
};

struct ProcessConfig {
    std::string inputDir;
    std::string outputDir;
    int threadCount;
    
    ProcessConfig()
        : threadCount(4)  // Default thread count
    {}
};

} // namespace trading 