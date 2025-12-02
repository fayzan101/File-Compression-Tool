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


// Helper for serialize
static void serializeNode(const shared_ptr<HuffmanNode>& node, vector<unsigned char>& out) {
    if (!node) return;
    if (!node->left && !node->right) {
        out.push_back(1); // Leaf marker
        out.push_back(node->byte);
    } else {
        out.push_back(0); // Internal node marker
        serializeNode(node->left, out);
        serializeNode(node->right, out);
    }
}

vector<unsigned char> HuffmanTree::serialize() const {
    vector<unsigned char> out;
    serializeNode(root, out);
    return out;
}

// Helper for deserialize
static shared_ptr<HuffmanNode> deserializeNode(const vector<unsigned char>& data, size_t& pos) {
    if (pos >= data.size()) throw std::runtime_error("Invalid serialization format: unexpected end of data");
    unsigned char marker = data[pos++];
    if (marker == 1) {
        if (pos >= data.size()) throw std::runtime_error("Invalid serialization format: missing symbol for leaf");
        unsigned char byte = data[pos++];
        auto node = make_shared<HuffmanNode>(byte, 0);
        node->left = nullptr;
        node->right = nullptr;
        return node;
    } else if (marker == 0) {
        auto left = deserializeNode(data, pos);
        auto right = deserializeNode(data, pos);
        // For deterministic parent, use min byte from children
        unsigned char parent_byte = std::min(left->byte, right->byte);
        auto parent = make_shared<HuffmanNode>(parent_byte, 0);
        parent->left = left;
        parent->right = right;
        return parent;
    } else {
        throw std::runtime_error("Invalid flag byte in serialization: " + std::to_string(marker));
    }
}

void HuffmanTree::deserialize(const vector<unsigned char>& data) {
    size_t pos = 0;
    root = deserializeNode(data, pos);
}

// Helper for DOT output - escape characters safely for Graphviz labels
static std::string escapeLabel(unsigned char c) {
    // Escape backslash and double-quote explicitly
    if (c == '\\') return "\\\\";   // one backslash in label
    if (c == '\"') return "\\\"";   // escaped quote in label

    // Printable ASCII range: keep as-is
    if (c >= 32 && c <= 126) {
        return std::string(1, static_cast<char>(c));
    }

    // Non-printable: render as \xHH
    char buf[5];
    std::snprintf(buf, sizeof(buf), "\\x%02X", c);
    return std::string(buf);
}

static void toDotNode(const shared_ptr<HuffmanNode>& node, std::ostream& out, std::unordered_map<const HuffmanNode*, int>& ids, int& idCounter) {
    if (!node) return;
    if (ids.find(node.get()) == ids.end()) ids[node.get()] = idCounter++;
    int id = ids[node.get()];
    bool isleaf = (!node->left && !node->right);
    if (isleaf) {
        out << "  node" << id << " [label=\"" << escapeLabel(node->byte) << "\\nFreq: " << node->freq << "\"];\n";
    } else {
        out << "  node" << id << " [label=\"Freq: " << node->freq << "\"];\n";
    }
    if (node->left) {
        toDotNode(node->left, out, ids, idCounter);
        out << "  node" << id << " -> node" << ids[node->left.get()] << " [label=\"0\"];\n";
    }
    if (node->right) {
        toDotNode(node->right, out, ids, idCounter);
        out << "  node" << id << " -> node" << ids[node->right.get()] << " [label=\"1\"];\n";
    }
}

std::string HuffmanTree::toDot() const {
    if (!root) return "digraph HuffmanTree{}\n";
    std::ostringstream out;
    out << "digraph HuffmanTree {\n";
    int idCounter = 0;
    std::unordered_map<const HuffmanNode*, int> ids;
    toDotNode(root, out, ids, idCounter);
    out << "}\n";
    return out.str();
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
