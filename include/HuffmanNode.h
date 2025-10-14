#pragma once

#include <memory>
#include <cstdint>

using namespace std;

struct HuffmanNode {
    unsigned char byte;
    uint64_t freq;
    shared_ptr<HuffmanNode> left;
    shared_ptr<HuffmanNode> right;

    HuffmanNode(unsigned char b, uint64_t f) : byte(b), freq(f) {}
    HuffmanNode(shared_ptr<HuffmanNode> l, shared_ptr<HuffmanNode> r) : byte(0), freq(l->freq + r->freq), left(l), right(r) {}
};
