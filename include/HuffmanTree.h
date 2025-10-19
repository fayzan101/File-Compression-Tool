#pragma once

#include "HuffmanNode.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

class HuffmanTree {
public:
    // Generate Graphviz DOT representation of the tree
    std::string toDot() const;
    using CodeTable = unordered_map<unsigned char, string>;
    using CodeLenTable = unordered_map<unsigned char, int>;

    HuffmanTree() = default;

    // build from frequency table
    void build(const unordered_map<unsigned char, uint64_t>& freq);

    // generate code table
    CodeTable getCodes() const;

    // canonical code table (for header)
    CodeLenTable getCodeLengths() const;
    CodeTable getCanonicalCodes() const;

    // serialize / deserialize (stubs)
    vector<unsigned char> serialize() const;
    void deserialize(const vector<unsigned char>& data);

private:
    shared_ptr<HuffmanNode> root;
    void buildCodes(const shared_ptr<HuffmanNode>& node, const string& prefix, CodeTable& table) const;
};
