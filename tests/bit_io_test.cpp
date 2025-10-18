#include "../include/BitWriter.h"
#include "../include/BitReader.h"
#include <cassert>
#include <vector>
#include <iostream>

int main() {
    std::cout << "Running BitWriter/BitReader unit tests..." << std::endl;
    
    // Test 1: Basic bit writing and reading
    {
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
        std::cout << "✓ Test 1 passed: Basic bit round-trip" << std::endl;
    }
    
    // Test 2: Multiple bytes
    {
        BitWriter writer;
        // Write 16 bits: 10110011 01001101
        writer.writeBit(1); writer.writeBit(0); writer.writeBit(1); writer.writeBit(1);
        writer.writeBit(0); writer.writeBit(0); writer.writeBit(1); writer.writeBit(1);
        writer.writeBit(0); writer.writeBit(1); writer.writeBit(0); writer.writeBit(0);
        writer.writeBit(1); writer.writeBit(1); writer.writeBit(0); writer.writeBit(1);
        writer.flush();
        
        const auto& buf = writer.getBuffer();
        assert(buf.size() == 2);
        assert(buf[0] == 0xB3);
        assert(buf[1] == 0x4D);
        
        BitReader reader(buf);
        std::vector<bool> bits;
        for (int i = 0; i < 16; ++i) bits.push_back(reader.readBit());
        std::vector<bool> expected = {1,0,1,1,0,0,1,1,0,1,0,0,1,1,0,1};
        assert(bits == expected);
        std::cout << "✓ Test 2 passed: Multiple bytes" << std::endl;
    }
    
    // Test 3: Partial byte (padding)
    {
        BitWriter writer;
        // Write 5 bits: 10110
        writer.writeBit(1);
        writer.writeBit(0);
        writer.writeBit(1);
        writer.writeBit(1);
        writer.writeBit(0);
        writer.flush();
        
        const auto& buf = writer.getBuffer();
        assert(buf.size() == 1);
        // Should be padded with zeros: 10110000 = 0xB0
        assert(buf[0] == 0xB0);
        
        BitReader reader(buf);
        std::vector<bool> bits;
        for (int i = 0; i < 5; ++i) bits.push_back(reader.readBit());
        std::vector<bool> expected = {1,0,1,1,0};
        assert(bits == expected);
        std::cout << "✓ Test 3 passed: Partial byte with padding" << std::endl;
    }
    
    // Test 4: writeBits function
    {
        BitWriter writer;
        // Write 0xAB (10101011) using writeBits
        writer.writeBits(0xAB, 8);
        writer.flush();
        
        const auto& buf = writer.getBuffer();
        assert(buf.size() == 1);
        assert(buf[0] == 0xAB);
        
        BitReader reader(buf);
        uint64_t value = reader.readBits(8);
        assert(value == 0xAB);
        std::cout << "✓ Test 4 passed: writeBits/readBits functions" << std::endl;
    }
    
    // Test 5: Edge case - empty buffer
    {
        BitWriter writer;
        writer.flush();
        const auto& buf = writer.getBuffer();
        assert(buf.empty());
        
        std::vector<uint8_t> empty_buf;
        BitReader reader(empty_buf);
        // Reading from empty buffer should return false
        assert(!reader.readBit());
        std::cout << "✓ Test 5 passed: Empty buffer handling" << std::endl;
    }
    
    std::cout << "All BitWriter/BitReader tests passed!" << std::endl;
    return 0;
}