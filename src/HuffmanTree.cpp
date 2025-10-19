#include "../include/HuffmanTree.h"
#include <queue>

void HuffmanTree::build(const unordered_map<unsigned char, uint64_t>& freq) {
    struct NodeCmp {
        bool operator()(const shared_ptr<HuffmanNode>& a, const shared_ptr<HuffmanNode>& b) const {
            if (a->freq != b->freq) return a->freq > b->freq;
            // break ties by symbol value (lowest byte first)
            return a->byte > b->byte;
        }
    };
    priority_queue<shared_ptr<HuffmanNode>, vector<shared_ptr<HuffmanNode>>, NodeCmp> pq;
    for (const auto& kv : freq) {
        pq.push(make_shared<HuffmanNode>(kv.first, kv.second));
    }
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        // For deterministic parent, use min byte from children
        unsigned char parent_byte = std::min(left->byte, right->byte);
        auto parent = make_shared<HuffmanNode>(parent_byte, left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        pq.push(parent);
    }
    root = pq.empty() ? nullptr : pq.top();
}

HuffmanTree::CodeTable HuffmanTree::getCodes() const {
    CodeTable table;
    buildCodes(root, "", table);
    return table;
}

void HuffmanTree::buildCodes(const shared_ptr<HuffmanNode>& node, const string& prefix, CodeTable& table) const {
    if (!node) return;
    if (!node->left && !node->right) {
        table[node->byte] = prefix.empty() ? "0" : prefix;
        return;
    }
    buildCodes(node->left, prefix + "0", table);
    buildCodes(node->right, prefix + "1", table);
}

vector<unsigned char> HuffmanTree::serialize() const {
    // Stub: not implemented
    return {};
}
void HuffmanTree::deserialize(const vector<unsigned char>& data) {
    // Stub: not implemented
}

HuffmanTree::CodeLenTable HuffmanTree::getCodeLengths() const {
    // Stub: not implemented
    return {};
}

HuffmanTree::CodeTable HuffmanTree::getCanonicalCodes() const {
    // Stub: not implemented
    return {};
}
