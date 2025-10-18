#include "../include/Compressor.h"
#include "../include/Decompressor.h"
#include "../include/CompressionSettings.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>

class PerformanceTest {
public:
    struct TestResult {
        std::string test_name;
        size_t original_size;
        size_t compressed_size;
        double compression_ratio;
        double compression_time_ms;
        double decompression_time_ms;
        bool success;
    };
    
    static TestResult runTest(const std::string& test_name, const std::string& data, 
                             const huffman::CompressionSettings& settings = huffman::make_settings_from_level(5)) {
        TestResult result;
        result.test_name = test_name;
        result.original_size = data.size();
        
        // Write test data
        std::ofstream test_file("perf_test_input.txt");
        test_file << data;
        test_file.close();
        
        // Test compression
        auto start = std::chrono::high_resolution_clock::now();
        Compressor comp;
        bool compress_result = comp.compress("perf_test_input.txt", "perf_test_compressed.huf", settings);
        auto compress_end = std::chrono::high_resolution_clock::now();
        
        if (!compress_result) {
            result.success = false;
            return result;
        }
        
        // Get compressed size
        std::ifstream compressed_file("perf_test_compressed.huf", std::ios::binary | std::ios::ate);
        result.compressed_size = compressed_file.tellg();
        compressed_file.close();
        
        // Test decompression
        auto decomp_start = std::chrono::high_resolution_clock::now();
        Decompressor decomp;
        bool decompress_result = decomp.decompress("perf_test_compressed.huf", "perf_test_output.txt");
        auto decomp_end = std::chrono::high_resolution_clock::now();
        
        if (!decompress_result) {
            result.success = false;
            return result;
        }
        
        // Verify round-trip
        std::ifstream output_file("perf_test_output.txt");
        std::string decompressed_data((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
        output_file.close();
        
        result.success = (data == decompressed_data);
        result.compression_ratio = (double)result.compressed_size / result.original_size * 100.0;
        result.compression_time_ms = std::chrono::duration<double, std::milli>(compress_end - start).count();
        result.decompression_time_ms = std::chrono::duration<double, std::milli>(decomp_end - decomp_start).count();
        
        return result;
    }
    
    static void printResults(const std::vector<TestResult>& results) {
        std::cout << "\n=== Performance Test Results ===" << std::endl;
        std::cout << std::left << std::setw(20) << "Test Name" 
                  << std::setw(12) << "Original" 
                  << std::setw(12) << "Compressed"
                  << std::setw(10) << "Ratio%"
                  << std::setw(12) << "Comp Time"
                  << std::setw(12) << "Decomp Time"
                  << std::setw(8) << "Status" << std::endl;
        std::cout << std::string(90, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(20) << result.test_name
                      << std::setw(12) << result.original_size
                      << std::setw(12) << result.compressed_size
                      << std::setw(10) << std::fixed << std::setprecision(1) << result.compression_ratio
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.compression_time_ms
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.decompression_time_ms
                      << std::setw(8) << (result.success ? "PASS" : "FAIL") << std::endl;
        }
    }
};

std::string generateRandomData(size_t size) {
    std::string data;
    data.reserve(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = 0; i < size; ++i) {
        data += static_cast<char>(dis(gen));
    }
    return data;
}

std::string generateRepetitiveData(size_t size) {
    std::string data;
    data.reserve(size);
    std::string pattern = "The quick brown fox jumps over the lazy dog. ";
    
    for (size_t i = 0; i < size; i += pattern.size()) {
        data += pattern;
    }
    return data.substr(0, size);
}

int main() {
    std::cout << "Running Performance Tests..." << std::endl;
    
    std::vector<PerformanceTest::TestResult> results;
    
    // Test 1: Small text
    results.push_back(PerformanceTest::runTest("Small Text", "Hello World!"));
    
    // Test 2: Medium text
    results.push_back(PerformanceTest::runTest("Medium Text", generateRepetitiveData(10000)));
    
    // Test 3: Large repetitive text
    results.push_back(PerformanceTest::runTest("Large Repetitive", generateRepetitiveData(100000)));
    
    // Test 4: Random data (small)
    results.push_back(PerformanceTest::runTest("Random Small", generateRandomData(1000)));
    
    // Test 5: Random data (medium)
    results.push_back(PerformanceTest::runTest("Random Medium", generateRandomData(10000)));
    
    // Test 6: Different compression levels
    std::string test_data = generateRepetitiveData(50000);
    results.push_back(PerformanceTest::runTest("Level 1", test_data, huffman::make_settings_from_level(1)));
    results.push_back(PerformanceTest::runTest("Level 5", test_data, huffman::make_settings_from_level(5)));
    results.push_back(PerformanceTest::runTest("Level 9", test_data, huffman::make_settings_from_level(9)));
    
    PerformanceTest::printResults(results);
    
    // Clean up
    std::remove("perf_test_input.txt");
    std::remove("perf_test_compressed.huf");
    std::remove("perf_test_output.txt");
    
    std::cout << "\nPerformance tests completed!" << std::endl;
    return 0;
}
