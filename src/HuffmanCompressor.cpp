#include "../include/HuffmanCompressor.h"
#include "../include/Compressor.h"
#include "../include/Decompressor.h"
#include "../include/Checksum.h"
#include <fstream>
#include <chrono>
#include <sstream>

namespace huffman {

CompressionResult compress(std::istream& in, std::ostream& out, const CompressionSettings& settings) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Read input data
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        result.original_size = data.size();
        result.original_checksum = CRC32::calculate(data);
        
        if (data.empty()) {
            // Handle empty input
            out.write("HUF1", 4);
            uint16_t table_size = 0;
            out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
            result.success = true;
            result.compressed_size = 6;
            return result;
        }
        
        // Write to temporary file and use existing compressor
        std::ofstream temp_in("temp_lib_input.txt", std::ios::binary);
        temp_in.write(reinterpret_cast<const char*>(data.data()), data.size());
        temp_in.close();
        
        Compressor comp;
        bool success = comp.compress("temp_lib_input.txt", "temp_lib_compressed.huf", settings);
        
        if (!success) {
            result.error = "Compression failed";
            return result;
        }
        
        // Read compressed data and write to output stream
        std::ifstream compressed_file("temp_lib_compressed.huf", std::ios::binary);
        std::vector<uint8_t> compressed_data((std::istreambuf_iterator<char>(compressed_file)), std::istreambuf_iterator<char>());
        compressed_file.close();
        
        out.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
        result.compressed_size = compressed_data.size();
        result.success = true;
        
        // Clean up
        std::remove("temp_lib_input.txt");
        std::remove("temp_lib_compressed.huf");
        
    } catch (const std::exception& e) {
        result.error = e.what();
        result.success = false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.compression_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.compression_ratio = result.original_size > 0 ? (double)result.compressed_size / result.original_size * 100.0 : 0.0;
    
    return result;
}

CompressionResult decompress(std::istream& in, std::ostream& out) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Read compressed data
        std::vector<uint8_t> compressed_data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        result.compressed_size = compressed_data.size();
        
        // Write to temporary file and use existing decompressor
        std::ofstream temp_compressed("temp_lib_compressed.huf", std::ios::binary);
        temp_compressed.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
        temp_compressed.close();
        
        Decompressor decomp;
        bool success = decomp.decompress("temp_lib_compressed.huf", "temp_lib_output.txt");
        
        if (!success) {
            result.error = "Decompression failed";
            return result;
        }
        
        // Read decompressed data and write to output stream
        std::ifstream output_file("temp_lib_output.txt", std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
        output_file.close();
        
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        result.original_size = data.size();
        result.success = true;
        
        // Clean up
        std::remove("temp_lib_compressed.huf");
        std::remove("temp_lib_output.txt");
        
    } catch (const std::exception& e) {
        result.error = e.what();
        result.success = false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.decompression_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

std::vector<uint8_t> compressBuffer(const std::vector<uint8_t>& in, const CompressionSettings& settings) {
    std::stringstream input_stream(std::ios::in | std::ios::out | std::ios::binary);
    input_stream.write(reinterpret_cast<const char*>(in.data()), in.size());
    
    std::stringstream output_stream(std::ios::in | std::ios::out | std::ios::binary);
    
    auto result = compress(input_stream, output_stream, settings);
    if (!result.success) {
        return {};
    }
    
    std::vector<uint8_t> compressed_data;
    output_stream.seekg(0, std::ios::end);
    size_t size = output_stream.tellg();
    output_stream.seekg(0, std::ios::beg);
    
    compressed_data.resize(size);
    output_stream.read(reinterpret_cast<char*>(compressed_data.data()), size);
    
    return compressed_data;
}

std::vector<uint8_t> decompressBuffer(const std::vector<uint8_t>& in) {
    std::stringstream input_stream(std::ios::in | std::ios::out | std::ios::binary);
    input_stream.write(reinterpret_cast<const char*>(in.data()), in.size());
    
    std::stringstream output_stream(std::ios::in | std::ios::out | std::ios::binary);
    
    auto result = decompress(input_stream, output_stream);
    if (!result.success) {
        return {};
    }
    
    std::vector<uint8_t> decompressed_data;
    output_stream.seekg(0, std::ios::end);
    size_t size = output_stream.tellg();
    output_stream.seekg(0, std::ios::beg);
    
    decompressed_data.resize(size);
    output_stream.read(reinterpret_cast<char*>(decompressed_data.data()), size);
    
    return decompressed_data;
}

CompressionResult compressFile(const std::string& inPath, const std::string& outPath, const CompressionSettings& settings) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Get original file size
        std::ifstream in(inPath, std::ios::binary | std::ios::ate);
        result.original_size = in.tellg();
        in.close();
        
        // Compress
        Compressor comp;
        bool success = comp.compress(inPath, outPath, settings);
        
        if (!success) {
            result.error = "Compression failed";
            return result;
        }
        
        // Get compressed file size
        std::ifstream out(outPath, std::ios::binary | std::ios::ate);
        result.compressed_size = out.tellg();
        out.close();
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error = e.what();
        result.success = false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.compression_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.compression_ratio = result.original_size > 0 ? (double)result.compressed_size / result.original_size * 100.0 : 0.0;
    
    return result;
}

CompressionResult decompressFile(const std::string& inPath, const std::string& outPath) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Get compressed file size
        std::ifstream in(inPath, std::ios::binary | std::ios::ate);
        result.compressed_size = in.tellg();
        in.close();
        
        // Decompress
        Decompressor decomp;
        bool success = decomp.decompress(inPath, outPath);
        
        if (!success) {
            result.error = "Decompression failed";
            return result;
        }
        
        // Get decompressed file size
        std::ifstream out(outPath, std::ios::binary | std::ios::ate);
        result.original_size = out.tellg();
        out.close();
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error = e.what();
        result.success = false;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.decompression_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

bool isValidCompressedFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    // Read up to 8 bytes (some magic values are 4, some 7 or 8)
    char magic[8] = {0};
    file.read(magic, 8);
    std::streamsize n = file.gcount();
    if (n <= 0) return false;

    std::string header(magic, static_cast<size_t>(n));

    // Accept any known magic/header variants produced by the compressor:
    // - legacy single-chunk empty file: "HUF1"
    // - legacy huffman: "HUF2"
    // - hybrid (LZ77 + Huffman): "HUF_LZ77"
    // - parallel container: "HUF_PAR"
    if (header.rfind("HUF1", 0) == 0) return true;
    if (header.rfind("HUF2", 0) == 0) return true;
    if (header.rfind("HUF_LZ77", 0) == 0) return true;
    if (header.rfind("HUF_PAR", 0) == 0) return true;

    return false;
}

size_t getCompressedFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return 0;
    return file.tellg();
}

std::string getVersion() {
    return "1.0.0";
}

} // namespace huffman
