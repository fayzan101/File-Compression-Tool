#include "../include/Compressor.h"
#include "../include/Decompressor.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>

bool testCompressDecompress(const std::string& test_data, const std::string& test_name) {
    std::cout << "Testing: " << test_name << std::endl;
    
    // Write test data to file
    std::ofstream test_file("temp_input.txt");
    test_file << test_data;
    test_file.close();
    
    // Compress
    Compressor comp;
    bool compress_result = comp.compress("temp_input.txt", "temp_compressed.huf");
    if (!compress_result) {
        std::cout << "  ❌ Compression failed!" << std::endl;
        return false;
    }
    
    // Decompress
    Decompressor decomp;
    bool decompress_result = decomp.decompress("temp_compressed.huf", "temp_output.txt");
    if (!decompress_result) {
        std::cout << "  ❌ Decompression failed!" << std::endl;
        return false;
    }
    
    // Verify round-trip
    std::ifstream output_file("temp_output.txt");
    std::string decompressed_data((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
    output_file.close();
    
    if (test_data != decompressed_data) {
        std::cout << "  ❌ Round-trip test failed!" << std::endl;
        std::cout << "    Expected: " << test_data << std::endl;
        std::cout << "    Got: " << decompressed_data << std::endl;
        return false;
    }
    
    // Check file sizes
    std::ifstream compressed_file("temp_compressed.huf", std::ios::binary | std::ios::ate);
    std::streamsize compressed_size = compressed_file.tellg();
    compressed_file.close();
    
    std::cout << "  ✓ Success! Original: " << test_data.size() 
              << " bytes, Compressed: " << compressed_size << " bytes" << std::endl;
    
    return true;
}

int main() {
    std::cout << "Running Integration Tests..." << std::endl;
    
    bool all_passed = true;
    
    // Test 1: Simple text
    all_passed &= testCompressDecompress("Hello World!", "Simple text");
    
    // Test 2: Empty string
    all_passed &= testCompressDecompress("", "Empty string");
    
    // Test 3: Single character repeated
    all_passed &= testCompressDecompress("aaaaaaaaaa", "Repeated character");
    
    // Test 4: All unique characters
    all_passed &= testCompressDecompress("abcdefghijklmnopqrstuvwxyz", "All unique characters");
    
    // Test 5: Binary-like data
    std::string binary_data;
    for (int i = 0; i < 256; ++i) {
        binary_data += static_cast<char>(i);
    }
    all_passed &= testCompressDecompress(binary_data, "Binary data (all bytes)");
    
    // Test 6: Large text
    std::string large_text;
    for (int i = 0; i < 1000; ++i) {
        large_text += "The quick brown fox jumps over the lazy dog. ";
    }
    all_passed &= testCompressDecompress(large_text, "Large repetitive text");
    
    // Test 7: Mixed case and special characters
    all_passed &= testCompressDecompress("Hello, World! 123 @#$%^&*()", "Mixed case and special chars");
    
    // Test 8: Newlines and whitespace
    all_passed &= testCompressDecompress("Line 1\nLine 2\r\nLine 3\tTabbed", "Newlines and whitespace");
    
    // Clean up
    std::remove("temp_input.txt");
    std::remove("temp_compressed.huf");
    std::remove("temp_output.txt");
    
    if (all_passed) {
        std::cout << "\n🎉 All integration tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some integration tests failed!" << std::endl;
        return 1;
    }
}

