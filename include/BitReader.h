#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

using namespace std;

class BitReader {
public:
    BitReader(const vector<unsigned char>& data);
    bool readBit(bool& bit);
    size_t bitCount() const;
private:
    const vector<unsigned char>& buffer;
    size_t bytePos;
    int bitPos;
    size_t bitsAvailable;
};
