#include "../include/Compressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitWriter.h"
#include "../include/ErrorHandler.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Compressor::compress(const std::string& inPath, const std::string& outPath) {
    return compress(inPath, outPath, huffman::make_settings_from_level(5));
}

bool Compressor::compress(const std::string& inPath, const std::string& outPath, const huffman::CompressionSettings& settings) {
    return compressInternal(inPath, outPath, settings);
}

bool Compressor::compressInternal(const std::string& inPath, const std::string& outPath, const huffman::CompressionSettings& settings) {
    try {
        // Read input file
        std::ifstream in(inPath, std::ios::binary);
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_NOT_FOUND, inPath);
        }
        
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (in.bad()) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_READ_ERROR, inPath);
        }
        
        // Handle empty file case
        if (data.empty()) {
            std::ofstream out(outPath, std::ios::binary);
            if (!out) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            // Write header for empty file: magic + table_size=0
            out.write("HUF1", 4);
            uint16_t table_size = 0;
            out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
            return true;
        }

        // Count frequencies
        std::unordered_map<unsigned char, uint64_t> freq;
        for (unsigned char c : data) freq[c]++;
        
        if (settings.verbose) {
            std::cout << "Compressing with level " << settings.level 
                      << " (mode: " << (settings.mode == huffman::CompressionSettings::FAST ? "FAST" : 
                                        settings.mode == huffman::CompressionSettings::DEFAULT ? "DEFAULT" : "BEST")
                      << ")" << std::endl;
            std::cout << "Input size: " << data.size() << " bytes" << std::endl;
            std::cout << "Unique symbols: " << freq.size() << std::endl;
        }

        // Build Huffman tree and code table
        HuffmanTree tree;
        tree.build(freq);

    auto codes = tree.getCodes();

        // Write to output file
        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }

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
        
        if (out.bad()) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }
        
        return true;
    } catch (const huffman::HuffmanError& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error during compression: " << e.what() << std::endl;
        return false;
    }
}
