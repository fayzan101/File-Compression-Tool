#include "../include/Decompressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitReader.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Decompressor::decompress(const std::string& inPath, const std::string& outPath) {
    // Read compressed file
    std::ifstream in(inPath, std::ios::binary);
    if (!in) return false;

    // Read magic
    char magic[4];
    in.read(magic, 4);
    if (in.gcount() != 4 || std::string(magic, 4) != "HUF1") return false;

    // Read frequency table size
    uint16_t table_size = 0;
    in.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
    if (!in) return false;

    // Read (symbol, freq) pairs
    std::unordered_map<unsigned char, uint64_t> freq;
    for (uint16_t i = 0; i < table_size; ++i) {
        int sym = in.get();
        if (sym == EOF) return false;
        uint64_t f = 0;
        in.read(reinterpret_cast<char*>(&f), sizeof(f));
        if (!in) return false;
        freq[static_cast<unsigned char>(sym)] = f;
    }

    // Read the rest as compressed data
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (buf.empty()) return false;

    // Build Huffman tree and code table
    HuffmanTree tree;
    tree.build(freq);

    auto codes = tree.getCodes();
    // Debug: print code table
    std::cout << "[Decompressor] Code table:\n";
    for (const auto& kv : codes) {
        std::cout << "  '" << kv.first << "' : " << kv.second << "\n";
    }

    // Build reverse code table for decoding
    std::unordered_map<std::string, unsigned char> rev_codes;
    for (const auto& kv : codes) rev_codes[kv.second] = kv.first;

    // Decode bitstream
    BitReader reader(buf);
    std::vector<unsigned char> outdata;
    std::string cur;
    // For each symbol in the original file, decode by traversing bits
    size_t total = 0;
    for (const auto& kv : freq) total += kv.second;
    for (size_t i = 0; i < total; ++i) {
        cur.clear();
        while (true) {
            bool bit = reader.readBit();
            cur += bit ? '1' : '0';
            auto it = rev_codes.find(cur);
            if (it != rev_codes.end()) {
                outdata.push_back(it->second);
                break;
            }
        }
    }

    // Write output file
    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(outdata.data()), outdata.size());
    return true;
}
