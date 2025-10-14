#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstddef>

using namespace std;

class BitWriter {
public:
    // default constructor
    BitWriter() : current(0), bitPos(0), bitsWritten(0) {}
    void writeBit(bool bit);
    void writeBits(const string& bits);
    const vector<unsigned char>& data() const;
    void flush();
    size_t bitCount() const;
private:
    vector<unsigned char> buffer;
    unsigned char current;
    int bitPos; // 0-7
    size_t bitsWritten;
};
