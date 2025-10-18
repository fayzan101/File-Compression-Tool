#include "../include/BitReader.h"
#include <vector>
#include <cstdint>

BitReader::BitReader(const std::vector<uint8_t>& buffer)
    : buffer_(buffer), byte_pos_(0), bit_pos_(0) {}

bool BitReader::readBit() {
    if (byte_pos_ >= buffer_.size()) return false;
    bool bit = (buffer_[byte_pos_] >> (7 - bit_pos_)) & 1;
    bit_pos_++;
    if (bit_pos_ == 8) {
        bit_pos_ = 0;
        byte_pos_++;
    }
    return bit;
}

uint64_t BitReader::readBits(unsigned count) {
    uint64_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        value = (value << 1) | (readBit() ? 1 : 0);
    }
    return value;
}

bool BitReader::hasMoreBits() const {
    return byte_pos_ < buffer_.size();
}