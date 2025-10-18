#include "../include/Compressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitWriter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Compressor::compress(const std::string& inPath, const std::string& outPath) {
    // Read input file
    std::ifstream in(inPath, std::ios::binary);
    if (!in) return false;
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.empty()) return false;

    // Count frequencies
    std::unordered_map<unsigned char, uint64_t> freq;
    for (unsigned char c : data) freq[c]++;

    // Build Huffman tree and code table
    HuffmanTree tree;
    tree.build(freq);

    auto codes = tree.getCodes();
    // Debug: print code table
    std::cout << "[Compressor] Code table:\n";
    for (const auto& kv : codes) {
        std::cout << "  '" << kv.first << "' : " << kv.second << "\n";
    }

    // Write to output file
    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;

    // Write header: magic, frequency table size, then (symbol, freq) pairs
    out.write("HUF1", 4); // magic
    uint16_t table_size = static_cast<uint16_t>(freq.size());
    out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
    for (const auto& kv : freq) {
        out.put(static_cast<char>(kv.first));
        uint64_t f = kv.second;
        out.write(reinterpret_cast<const char*>(&f), sizeof(f));
    }

    // Write encoded output
    BitWriter writer;
    for (unsigned char c : data) {
        const std::string& bits = codes[c];
        for (char b : bits) writer.writeBit(b == '1');
    }
    writer.flush();
    const auto& buf = writer.getBuffer();
    out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    return true;
}
