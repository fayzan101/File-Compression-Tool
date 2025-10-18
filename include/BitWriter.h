#pragma once
#include <vector>
#include <cstdint>
#include <ostream>

class BitWriter {
public:
    BitWriter();
    void writeBit(bool bit);
    void writeBits(uint64_t value, unsigned count);
    void flush();
    const std::vector<uint8_t>& getBuffer() const;
    void writeToStream(std::ostream& os);
private:
    std::vector<uint8_t> buffer_;
    uint8_t current_byte_;
    int bit_pos_;
};
