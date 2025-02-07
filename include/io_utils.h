#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace trading {
namespace io {

class FileReader {
public:
    static constexpr size_t BUFFER_SIZE = 512 * 1024 * 1024; // 512MB buffer
    static constexpr size_t MIN_REMAINING = 10000;

    FileReader(const std::string& path);
    bool readChunk();
    const char* current() const { return content_.data() + current_pos_; }
    const char* end() const { return content_.data() + content_.size(); }
    void advance(size_t offset) {
        size_t new_pos = current_pos_ + offset;
        if (new_pos > content_.size()) {
            throw std::runtime_error("Invalid position advancement");
        }
        current_pos_ = new_pos;
    }
    bool eof() const { return eof_; }

private:
    std::ifstream file_;
    std::string content_;
    std::vector<char> buffer_;
    size_t current_pos_{0};
    bool eof_{false};

    void compactContent();
};

class FastFormatter {
public:
    static char* formatInt(char* buf, int64_t val);
    static char* formatDouble(char* buf, double val, int precision);
};

} // namespace io
} // namespace trading 