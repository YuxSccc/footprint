#pragma once
#include "trade.h"
#include <string>
#include <memory>

namespace trading {

class DataFetcher {
public:
    virtual ~DataFetcher() = default;
    
    // Parse a single line of data into a Trade object
    virtual Trade parseLine(const char* start, const char** end) = 0;
    
    // Check if the line is a header
    virtual bool isHeader(const char* line) = 0;
    
    // Factory method to create appropriate fetcher based on exchange name
    static std::unique_ptr<DataFetcher> create(const std::string& exchange);
};

class BinanceFetcher : public DataFetcher {
public:
    Trade parseLine(const char* start, const char** end) override;
    bool isHeader(const char* line) override;

private:
    static int64_t fastAtoll(const char* str, const char** end);
    static double fastAtof(const char* str, const char** end);
};

} // namespace trading 