#pragma once

#include <string>
#include "CompressionSettings.h"

using namespace std;

class Compressor {
public:
    bool compress(const string& inPath, const string& outPath);
    bool compress(const string& inPath, const string& outPath, const huffman::CompressionSettings& settings);
    bool compressParallel(const string& inPath, const string& outPath, const huffman::CompressionSettings& settings, size_t chunkSize);

private:
    bool compressInternal(const string& inPath, const string& outPath, const huffman::CompressionSettings& settings);
};
