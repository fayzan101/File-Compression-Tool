#pragma once
#include <vector>
#include <cstdint>

class BitReader {
public:
    BitReader(const std::vector<uint8_t>& buffer);
    bool readBit();
    uint64_t readBits(unsigned count);
private:
    const std::vector<uint8_t>& buffer_;
    size_t byte_pos_;
    int bit_pos_;
};
