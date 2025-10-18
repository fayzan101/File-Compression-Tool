#include "../include/BitWriter.h"
#include <vector>
#include <cstdint>
#include <ostream>

BitWriter::BitWriter() : buffer_(), current_byte_(0), bit_pos_(0) {}

void BitWriter::writeBit(bool bit) {
    current_byte_ = (current_byte_ << 1) | (bit ? 1 : 0);
    bit_pos_++;
    if (bit_pos_ == 8) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bit_pos_ = 0;
    }
}

void BitWriter::writeBits(uint64_t value, unsigned count) {
    for (int i = count - 1; i >= 0; --i) {
        writeBit((value >> i) & 1);
    }
}

void BitWriter::flush() {
    if (bit_pos_ > 0) {
        buffer_.push_back(current_byte_ << (8 - bit_pos_));
        current_byte_ = 0;
        bit_pos_ = 0;
    }
}

const std::vector<uint8_t>& BitWriter::getBuffer() const {
    return buffer_;
}

void BitWriter::writeToStream(std::ostream& os) {
    flush();
    for (uint8_t b : buffer_) os.put(static_cast<char>(b));
}
