#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace huffman {

class CRC32 {
public:
    static uint32_t calculate(const std::vector<uint8_t>& data);
    static uint32_t calculate(const std::string& data);
    static uint32_t calculate(const uint8_t* data, size_t length);
    static std::string toHex(uint32_t crc);
    static uint32_t fromHex(const std::string& hex);
};

} // namespace huffman

