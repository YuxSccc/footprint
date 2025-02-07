#pragma once
#include "footprint.h"
#include "config.h"
#include <string>
#include <memory>

namespace trading {

class OutputHandler {
public:
    virtual ~OutputHandler() = default;
    virtual void write(const std::string& filename, 
                      const std::vector<FootprintBar>& bars,
                      const SymbolConfig& config) = 0;
    
    static std::unique_ptr<OutputHandler> create(const std::string& format);
};

class JsonOutputHandler : public OutputHandler {
public:
    void write(const std::string& filename,
              const std::vector<FootprintBar>& bars,
              const SymbolConfig& config) override;
};

class CsvOutputHandler : public OutputHandler {
public:
    void write(const std::string& filename,
              const std::vector<FootprintBar>& bars,
              const SymbolConfig& config) override;
};

} // namespace trading 