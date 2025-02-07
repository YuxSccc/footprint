#include "io_utils.h"
#include <stdexcept>
#include <cstring>
#include <cmath>

namespace trading {
namespace io {

FileReader::FileReader(const std::string& path) 
    : file_(path, std::ios::binary)
    , buffer_(BUFFER_SIZE) {
    if (!file_) {
        throw std::runtime_error("Failed to open file: " + path);
    }
}

bool FileReader::readChunk() {
    // 如果已处理数据超过buffer大小的一半，进行压缩
    if (current_pos_ > BUFFER_SIZE / 2) {
        compactContent();
    }

    // 读取新数据
    if (file_.read(buffer_.data(), BUFFER_SIZE) || file_.gcount() > 0) {
        size_t read_size = file_.gcount();
        content_.append(buffer_.data(), read_size);

        if (read_size < BUFFER_SIZE) {
            eof_ = true;
        }
    } else {
        eof_ = true;
    }

    return !content_.empty();
}

void FileReader::compactContent() {
    if (current_pos_ > 0) {
        content_.erase(0, current_pos_);
        current_pos_ = 0;
    }
}

char* FastFormatter::formatInt(char* buf, int64_t val) {
    char tmp[32];
    char* p = tmp + sizeof(tmp);
    *--p = '\0';
    bool neg = val < 0;
    uint64_t uval = neg ? -val : val;

    do {
        *--p = '0' + (uval % 10);
        uval /= 10;
    } while (uval > 0);

    if (neg) *--p = '-';
    size_t len = tmp + sizeof(tmp) - p - 1;
    memcpy(buf, p, len);
    return buf + len;
}

char* FastFormatter::formatDouble(char* buf, double val, int precision) {
    int64_t int_part = static_cast<int64_t>(val);
    char* p = formatInt(buf, int_part);
    *p++ = '.';

    double frac_part = val - int_part;
    if (frac_part < 0) frac_part = -frac_part;
    int64_t scaled = static_cast<int64_t>(frac_part * std::pow(10, precision) + 0.5);

    // 补齐前导零
    int zeros = precision - 1;
    while (scaled < std::pow(10, zeros) && zeros > 0) {
        *p++ = '0';
        zeros--;
    }

    return formatInt(p, scaled);
}

} // namespace io
} // namespace trading 