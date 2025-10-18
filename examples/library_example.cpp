#include "../include/HuffmanCompressor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

int main() {
    std::cout << "Huffman Compression Library Example" << std::endl;
    std::cout << "Version: " << huffman::getVersion() << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Example 1: File compression with different levels
    std::cout << "\n1. File Compression Example" << std::endl;
    std::string test_data = "The quick brown fox jumps over the lazy dog. This is a test of Huffman compression.";
    
    // Write test data
    std::ofstream test_file("example_input.txt");
    test_file << test_data;
    test_file.close();
    
    // Test different compression levels
    for (int level = 1; level <= 9; level += 4) {
        std::string output_file = "example_compressed_level_" + std::to_string(level) + ".huf";
        auto settings = huffman::make_settings_from_level(level);
        settings.verbose = true;
        
        auto result = huffman::compressFile("example_input.txt", output_file, settings);
        
        std::cout << "Level " << level << ": " 
                  << result.original_size << " -> " << result.compressed_size 
                  << " bytes (" << std::fixed << std::setprecision(1) << result.compression_ratio << "%)"
                  << " in " << result.compression_time_ms << "ms" << std::endl;
    }
    
    // Example 2: Buffer compression
    std::cout << "\n2. Buffer Compression Example" << std::endl;
    std::vector<uint8_t> input_buffer(test_data.begin(), test_data.end());
    auto compressed_buffer = huffman::compressBuffer(input_buffer);
    auto decompressed_buffer = huffman::decompressBuffer(compressed_buffer);
    
    std::cout << "Original buffer size: " << input_buffer.size() << " bytes" << std::endl;
    std::cout << "Compressed buffer size: " << compressed_buffer.size() << " bytes" << std::endl;
    std::cout << "Decompressed buffer size: " << decompressed_buffer.size() << " bytes" << std::endl;
    
    // Verify round-trip
    bool round_trip_success = (input_buffer == decompressed_buffer);
    std::cout << "Round-trip test: " << (round_trip_success ? "PASS" : "FAIL") << std::endl;
    
    // Example 3: Stream compression
    std::cout << "\n3. Stream Compression Example" << std::endl;
    std::stringstream input_stream;
    input_stream << test_data;
    
    std::stringstream compressed_stream;
    auto stream_result = huffman::compress(input_stream, compressed_stream);
    
    std::cout << "Stream compression: " 
              << stream_result.original_size << " -> " << stream_result.compressed_size 
              << " bytes in " << stream_result.compression_time_ms << "ms" << std::endl;
    
    // Example 4: File validation
    std::cout << "\n4. File Validation Example" << std::endl;
    std::cout << "Is 'example_compressed_level_5.huf' valid? " 
              << (huffman::isValidCompressedFile("example_compressed_level_5.huf") ? "YES" : "NO") << std::endl;
    std::cout << "Is 'example_input.txt' valid? " 
              << (huffman::isValidCompressedFile("example_input.txt") ? "YES" : "NO") << std::endl;
    
    // Example 5: Decompression
    std::cout << "\n5. Decompression Example" << std::endl;
    auto decomp_result = huffman::decompressFile("example_compressed_level_5.huf", "example_decompressed.txt");
    
    if (decomp_result.success) {
        std::cout << "Decompression successful: " 
                  << decomp_result.compressed_size << " -> " << decomp_result.original_size 
                  << " bytes in " << decomp_result.decompression_time_ms << "ms" << std::endl;
        
        // Verify content
        std::ifstream decomp_file("example_decompressed.txt");
        std::string decomp_content((std::istreambuf_iterator<char>(decomp_file)), std::istreambuf_iterator<char>());
        decomp_file.close();
        
        std::cout << "Content matches original: " << (decomp_content == test_data ? "YES" : "NO") << std::endl;
    } else {
        std::cout << "Decompression failed: " << decomp_result.error << std::endl;
    }
    
    // Clean up
    std::remove("example_input.txt");
    std::remove("example_compressed_level_1.huf");
    std::remove("example_compressed_level_5.huf");
    std::remove("example_compressed_level_9.huf");
    std::remove("example_decompressed.txt");
    
    std::cout << "\nExample completed successfully!" << std::endl;
    return 0;
}
