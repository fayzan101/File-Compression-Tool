#pragma once

#include <string>
#include "CompressionSettings.h"

using namespace std;

class Compressor {
public:
    bool compress(const string& inPath, const string& outPath);
    bool compress(const string& inPath, const string& outPath, const huffman::CompressionSettings& settings);
    
private:
    bool compressInternal(const string& inPath, const string& outPath, const huffman::CompressionSettings& settings);
};
