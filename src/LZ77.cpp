#include "../include/LZ77.h"
#include <algorithm>

std::vector<LZ77::Token> LZ77::compress(const std::vector<uint8_t>& data, size_t window, size_t lookahead) {
    std::vector<Token> tokens;
    size_t pos = 0;
    while (pos < data.size()) {
        size_t best_offset = 0, best_length = 0;
        size_t start = pos >= window ? pos - window : 0;
        for (size_t i = start; i < pos; ++i) {
            size_t len = 0;
            while (len < lookahead && pos + len < data.size() && data[i + len] == data[pos + len]) {
                ++len;
            }
            if (len > best_length) {
                best_length = len;
                best_offset = pos - i;
            }
        }
        uint8_t next = pos + best_length < data.size() ? data[pos + best_length] : 0;
        tokens.push_back({(uint16_t)best_offset, (uint16_t)best_length, next});
        pos += best_length + 1;
    }
    return tokens;
}

std::vector<uint8_t> LZ77::decompress(const std::vector<Token>& tokens) {
    std::vector<uint8_t> out;
    for (const auto& t : tokens) {
        size_t start = out.size() >= t.offset ? out.size() - t.offset : 0;
        for (size_t i = 0; i < t.length; ++i) {
            out.push_back(out[start + i]);
        }
        out.push_back(t.next);
    }
    return out;
}

std::vector<uint8_t> LZ77::tokensToBytes(const std::vector<Token>& tokens) {
    std::vector<uint8_t> bytes;
    for (const auto& t : tokens) {
        bytes.push_back(t.offset >> 8);
        bytes.push_back(t.offset & 0xFF);
        bytes.push_back(t.length >> 8);
        bytes.push_back(t.length & 0xFF);
        bytes.push_back(t.next);
    }
    return bytes;
}

std::vector<LZ77::Token> LZ77::bytesToTokens(const std::vector<uint8_t>& bytes) {
    std::vector<Token> tokens;
    for (size_t i = 0; i + 4 < bytes.size(); i += 5) {
        uint16_t offset = (bytes[i] << 8) | bytes[i + 1];
        uint16_t length = (bytes[i + 2] << 8) | bytes[i + 3];
        uint8_t next = bytes[i + 4];
        tokens.push_back({offset, length, next});
    }
    return tokens;
}
