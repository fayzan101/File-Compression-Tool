#pragma once

#include <string>
using namespace std;

class Decompressor {
public:
    bool decompress(const string& inPath, const string& outPath);
};
