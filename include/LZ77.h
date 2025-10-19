#pragma once
#include <vector>
#include <cstdint>
#include <string>

class LZ77 {
public:
    struct Token {
        uint16_t offset;
        uint16_t length;
        uint8_t next;
    };
    static std::vector<Token> compress(const std::vector<uint8_t>& data, size_t window = 4096, size_t lookahead = 18);
    static std::vector<uint8_t> decompress(const std::vector<Token>& tokens);
    static std::vector<uint8_t> tokensToBytes(const std::vector<Token>& tokens);
    static std::vector<Token> bytesToTokens(const std::vector<uint8_t>& bytes);
};
