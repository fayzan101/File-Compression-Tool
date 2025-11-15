#include "../include/HuffmanTree.h"
#include <queue>
#include <algorithm>
#include <sstream>

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
    CodeLenTable lens;
    if (!root) return lens;
    CodeTable codes = getCodes();
    for (const auto& kv : codes) {
        lens[kv.first] = static_cast<int>(kv.second.size());
    }
    return lens;
}

HuffmanTree::CodeTable HuffmanTree::getCanonicalCodes() const {
    CodeTable result;
    auto lens = getCodeLengths();
    if (lens.empty()) return result;

    // Build list sorted by length then symbol
    std::vector<std::pair<unsigned char, int>> items;
    for (const auto& kv : lens) items.push_back(kv);
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second < b.second;
        return a.first < b.first;
    });

    // Assign canonical codes
    unsigned int code = 0;
    int prev_len = items.front().second;
    for (size_t i = 0; i < items.size(); ++i) {
        int len = items[i].second;
        if (i > 0) {
            ++code;
            if (len > prev_len) code <<= (len - prev_len);
        }
        prev_len = len;
        // Compute bitstring of length `len` from `code`
        std::string bits;
        for (int b = len - 1; b >= 0; --b) bits.push_back(((code >> b) & 1) ? '1' : '0');
        result[items[i].first] = bits;
    }

    return result;
}
