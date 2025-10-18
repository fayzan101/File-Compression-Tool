#include "../include/HuffmanTree.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <string>

bool isPrefixFree(const std::unordered_map<unsigned char, std::string>& codes) {
    for (const auto& kv1 : codes) {
        for (const auto& kv2 : codes) {
            if (kv1.first != kv2.first) {
                const std::string& code1 = kv1.second;
                const std::string& code2 = kv2.second;
                if (code1.length() < code2.length() && 
                    code2.substr(0, code1.length()) == code1) {
                    return false; // code1 is a prefix of code2
                }
            }
        }
    }
    return true;
}

int main() {
    std::cout << "Running HuffmanTree unit tests..." << std::endl;
    
    // Test 1: Simple frequency table
    {
        std::unordered_map<unsigned char, uint64_t> freq = {
            {'a', 5}, {'b', 2}, {'c', 1}
        };
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.size() == 3);
        assert(isPrefixFree(codes));
        
        // Most frequent should have shortest code
        assert(codes['a'].length() <= codes['b'].length());
        assert(codes['a'].length() <= codes['c'].length());
        
        std::cout << "✓ Test 1 passed: Simple frequency table" << std::endl;
        std::cout << "  Codes: a=" << codes['a'] << ", b=" << codes['b'] << ", c=" << codes['c'] << std::endl;
    }
    
    // Test 2: Single character
    {
        std::unordered_map<unsigned char, uint64_t> freq = {
            {'x', 10}
        };
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.size() == 1);
        assert(codes['x'] == "0"); // Single character gets code "0"
        
        std::cout << "✓ Test 2 passed: Single character" << std::endl;
    }
    
    // Test 3: Two characters
    {
        std::unordered_map<unsigned char, uint64_t> freq = {
            {'A', 3}, {'B', 7}
        };
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.size() == 2);
        assert(isPrefixFree(codes));
        // Should have codes "0" and "1"
        assert(codes['A'].length() == 1);
        assert(codes['B'].length() == 1);
        assert(codes['A'] != codes['B']);
        
        std::cout << "✓ Test 3 passed: Two characters" << std::endl;
        std::cout << "  Codes: A=" << codes['A'] << ", B=" << codes['B'] << std::endl;
    }
    
    // Test 4: All characters have same frequency
    {
        std::unordered_map<unsigned char, uint64_t> freq = {
            {'a', 1}, {'b', 1}, {'c', 1}, {'d', 1}
        };
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.size() == 4);
        assert(isPrefixFree(codes));
        
        // All should have same length (balanced tree)
        int code_length = codes['a'].length();
        for (const auto& kv : codes) {
            assert(kv.second.length() == code_length);
        }
        
        std::cout << "✓ Test 4 passed: Equal frequencies" << std::endl;
    }
    
    // Test 5: Empty frequency table
    {
        std::unordered_map<unsigned char, uint64_t> freq;
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.empty());
        
        std::cout << "✓ Test 5 passed: Empty frequency table" << std::endl;
    }
    
    // Test 6: Deterministic behavior (same input should produce same output)
    {
        std::unordered_map<unsigned char, uint64_t> freq = {
            {'x', 1}, {'y', 2}, {'z', 3}
        };
        
        HuffmanTree tree1, tree2;
        tree1.build(freq);
        tree2.build(freq);
        
        auto codes1 = tree1.getCodes();
        auto codes2 = tree2.getCodes();
        
        assert(codes1 == codes2); // Should be identical
        
        std::cout << "✓ Test 6 passed: Deterministic behavior" << std::endl;
    }
    
    // Test 7: Large frequency table
    {
        std::unordered_map<unsigned char, uint64_t> freq;
        for (int i = 0; i < 26; ++i) {
            freq['a' + i] = i + 1; // 'a'=1, 'b'=2, ..., 'z'=26
        }
        
        HuffmanTree tree;
        tree.build(freq);
        auto codes = tree.getCodes();
        
        assert(codes.size() == 26);
        assert(isPrefixFree(codes));
        
        // Most frequent should have shortest code
        assert(codes['z'].length() <= codes['a'].length());
        
        std::cout << "✓ Test 7 passed: Large frequency table (26 characters)" << std::endl;
    }
    
    std::cout << "All HuffmanTree tests passed!" << std::endl;
    return 0;
}
