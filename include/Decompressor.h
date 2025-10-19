#pragma once

#include <string>

class Decompressor {
public:
    bool decompress(const std::string& inPath, const std::string& outPath);
};
