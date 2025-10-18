#include "../include/Decompressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitReader.h"
#include "../include/ErrorHandler.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Decompressor::decompress(const std::string& inPath, const std::string& outPath) {
    try {
        // Read compressed file
        std::ifstream in(inPath, std::ios::binary);
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_NOT_FOUND, inPath);
        }

        // Read magic
        char magic[4];
        in.read(magic, 4);
        if (in.gcount() != 4) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read magic number");
        }
        if (std::string(magic, 4) != "HUF1") {
            throw huffman::HuffmanError(huffman::ErrorCode::INVALID_MAGIC, std::string(magic, 4));
        }

        // Read frequency table size
        uint16_t table_size = 0;
        in.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read table size");
        }

        // Handle empty file case
        if (table_size == 0) {
            std::ofstream out(outPath, std::ios::binary);
            if (!out) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            // Empty file - just create empty output
            return true;
        }

        // Read (symbol, freq) pairs
        std::unordered_map<unsigned char, uint64_t> freq;
        for (uint16_t i = 0; i < table_size; ++i) {
            int sym = in.get();
            if (sym == EOF) {
                throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Unexpected end of file while reading frequency table");
            }
            uint64_t f = 0;
            in.read(reinterpret_cast<char*>(&f), sizeof(f));
            if (!in) {
                throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read frequency for symbol " + std::to_string(sym));
            }
            freq[static_cast<unsigned char>(sym)] = f;
        }

        // Read the rest as compressed data
        std::vector<unsigned char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (buf.empty()) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "No compressed data found");
        }

    // Build Huffman tree and code table
    HuffmanTree tree;
    tree.build(freq);

    auto codes = tree.getCodes();

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
            // Check if we can read a bit
            if (!reader.hasMoreBits()) {
                return false; // Unexpected end of stream
            }
            bool bit = reader.readBit();
            // If readBit() returned false due to end of stream, we should have caught it above
            // So this bit is valid
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
        if (!out) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }
        out.write(reinterpret_cast<const char*>(outdata.data()), outdata.size());
        
        if (out.bad()) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }
        
        return true;
    } catch (const huffman::HuffmanError& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error during decompression: " << e.what() << std::endl;
        return false;
    }
}
