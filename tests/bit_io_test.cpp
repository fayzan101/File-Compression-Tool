#include "../include/BitWriter.h"
#include "../include/BitReader.h"
#include <cassert>
#include <vector>
#include <iostream>

int main() {
    BitWriter writer;
    // Write the bit pattern 10110011 (0xB3)
    writer.writeBit(1);
    writer.writeBit(0);
    writer.writeBit(1);
    writer.writeBit(1);
    writer.writeBit(0);
    writer.writeBit(0);
    writer.writeBit(1);
    writer.writeBit(1);
    writer.flush();
    const auto& buf = writer.getBuffer();
    assert(buf.size() == 1 && buf[0] == 0xB3);

    BitReader reader(buf);
    std::vector<bool> bits;
    for (int i = 0; i < 8; ++i) bits.push_back(reader.readBit());
    std::vector<bool> expected = {1,0,1,1,0,0,1,1};
    assert(bits == expected);
    std::cout << "BitWriter/BitReader round-trip test passed.\n";
    return 0;
}
