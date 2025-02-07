#include "processor.h"
#include <thread>
#include <semaphore>
#include <filesystem>

namespace fs = std::filesystem;

constexpr int max_thread_count = 64;

int main() {
    trading::ProcessConfig processConfig;
    processConfig.inputDir = "/mnt/d/orderdata/binance/unsorted_rawdata";
    processConfig.outputDir = "/mnt/d/orderdata/binance/";
    
    trading::SymbolConfig symbolConfig;
    
    auto fetcher = trading::DataFetcher::create("binance");
    auto outputHandler = trading::OutputHandler::create("json");
    
    trading::Processor processor(
        std::move(fetcher),
        std::move(outputHandler),
        processConfig,
        symbolConfig
    );
    
    processConfig.threadCount = std::min(max_thread_count, processConfig.threadCount);

    std::vector<std::thread> threads;
    std::counting_semaphore<max_thread_count> sem(processConfig.threadCount);
    
    for (const auto& entry : fs::directory_iterator(processConfig.inputDir)) {
        if (entry.path().extension() != ".csv") continue;
        
        threads.emplace_back([&sem, &processor, entry]() {
            sem.acquire();
            processor.processFile(entry.path().string());
            sem.release();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
} 