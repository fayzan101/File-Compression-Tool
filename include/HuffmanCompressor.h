#pragma once

#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include "CompressionSettings.h"

using namespace std;

namespace huffman {

struct CompressionResult {
    size_t original_size = 0;
    size_t compressed_size = 0;
    bool success = false;
    std::string error;
};

// Stream-based API
CompressionResult compress(std::istream& in, std::ostream& out, const CompressionSettings& settings = CompressionSettings());
CompressionResult decompress(std::istream& in, std::ostream& out);

// Buffer API
std::vector<uint8_t> compressBuffer(const std::vector<uint8_t>& in, const CompressionSettings& settings = CompressionSettings());
std::vector<uint8_t> decompressBuffer(const std::vector<uint8_t>& in);

} // namespace huffman
