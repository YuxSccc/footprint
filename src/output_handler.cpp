#include "output_handler.h"
#include "json.hpp"
#include <fstream>
#include <filesystem>

namespace trading {

namespace fs = std::filesystem;
using json = nlohmann::json;

std::unique_ptr<OutputHandler> OutputHandler::create(const std::string& format) {
    if (format == "json") {
        return std::make_unique<JsonOutputHandler>();
    } else if (format == "csv") {
        return std::make_unique<CsvOutputHandler>();
    }
    throw std::runtime_error("Unsupported output format: " + format);
}

void JsonOutputHandler::write(
    const std::string& filename,
    const std::vector<FootprintBar>& bars,
    [[maybe_unused]] const SymbolConfig& config) {
    
    fs::path outputPath = filename;
    outputPath.replace_extension(".json");
    
    // 确保输出目录存在
    fs::create_directories(outputPath.parent_path());
    
    json outputJson;
    for (const auto& bar : bars) {
        outputJson[std::to_string(bar.timestamp)] = json::parse(bar.toJson());
    }
    
    std::ofstream outFile(outputPath);
    if (!outFile) {
        throw std::runtime_error("Failed to open output file: " + outputPath.string());
    }
    
    outFile << outputJson.dump(4);
}

void CsvOutputHandler::write(
    const std::string& filename,
    const std::vector<FootprintBar>& bars,
    const SymbolConfig& config) {
    
    // TODO: Implement CSV output format
    throw std::runtime_error("CSV output format not implemented yet");
}

} // namespace trading 