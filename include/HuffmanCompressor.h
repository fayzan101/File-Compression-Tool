#pragma once

#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include "CompressionSettings.h"
#include "ErrorHandler.h"

using namespace std;

namespace huffman {

struct CompressionResult {
    size_t original_size = 0;
    size_t compressed_size = 0;
    bool success = false;
    std::string error;
    double compression_ratio = 0.0;
    double compression_time_ms = 0.0;
    double decompression_time_ms = 0.0;
    uint32_t original_checksum = 0;
    uint32_t compressed_checksum = 0;
    bool checksum_verified = false;
};

// Stream-based API
CompressionResult compress(std::istream& in, std::ostream& out, const CompressionSettings& settings = CompressionSettings());
CompressionResult decompress(std::istream& in, std::ostream& out);

// Buffer API
std::vector<uint8_t> compressBuffer(const std::vector<uint8_t>& in, const CompressionSettings& settings = CompressionSettings());
std::vector<uint8_t> decompressBuffer(const std::vector<uint8_t>& in);

// File API with detailed results
CompressionResult compressFile(const std::string& inPath, const std::string& outPath, const CompressionSettings& settings = CompressionSettings());
CompressionResult decompressFile(const std::string& inPath, const std::string& outPath);

// Utility functions
bool isValidCompressedFile(const std::string& path);
size_t getCompressedFileSize(const std::string& path);
std::string getVersion();

} // namespace huffman