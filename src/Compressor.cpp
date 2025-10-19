
#include <fstream>
#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "../include/Compressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitWriter.h"
#include "../include/ErrorHandler.h"
#include "../include/Checksum.h"

// Helper: Split data into chunks
static std::vector<std::vector<unsigned char>> splitChunks(const std::vector<unsigned char>& data, size_t chunkSize) {
    std::vector<std::vector<unsigned char>> chunks;
    size_t total = data.size();
    for (size_t i = 0; i < total; i += chunkSize) {
        size_t end = std::min(i + chunkSize, total);
        chunks.emplace_back(data.begin() + i, data.begin() + end);
    }
    return chunks;
}

// Parallel compress function
bool Compressor::compressParallel(const std::string& inPath, const std::string& outPath, const huffman::CompressionSettings& settings, size_t chunkSize) {
    try {
        std::ifstream in(inPath, std::ios::binary);
    if (!in) throw huffman::HuffmanError(huffman::ErrorCode::FILE_NOT_FOUND, inPath);
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) throw huffman::HuffmanError(huffman::ErrorCode::FILE_READ_ERROR, inPath);
        if (data.empty()) return compressInternal(inPath, outPath, settings);

        // Split into chunks
        auto chunks = splitChunks(data, chunkSize);
        size_t numChunks = chunks.size();
        std::vector<std::vector<unsigned char>> compressedChunks(numChunks);
        std::vector<size_t> chunkSizes(numChunks);

        // Compress each chunk in parallel
        std::vector<std::future<void>> futures;
        std::mutex mtx;
        for (size_t i = 0; i < numChunks; ++i) {
            futures.push_back(std::async(std::launch::async, [&, i]() {
                // Use a local Compressor for each chunk
                Compressor localCompressor;
                std::string tempOut = "chunk_" + std::to_string(i) + ".huf";
                // Compress chunk to buffer (not file)
                std::vector<unsigned char>& chunkData = chunks[i];
                HuffmanTree tree;
                std::unordered_map<unsigned char, uint64_t> freq;
                for (unsigned char c : chunkData) freq[c]++;
                tree.build(freq);
                auto canonical_codes = tree.getCanonicalCodes();
                BitWriter writer;
                for (unsigned char c : chunkData) {
                    const std::string& bits = canonical_codes[c];
                    for (char b : bits) writer.writeBit(b == '1');
                }
                writer.flush();
                const auto& buf = writer.getBuffer();
                uint32_t crc = huffman::CRC32::calculate(buf);
                std::vector<unsigned char> outbuf;
                // Write header: magic + code lengths + CRC32 + compressed data
                outbuf.insert(outbuf.end(), {'H','U','F','2'});
                HuffmanTree::CodeLenTable code_lens = tree.getCodeLengths();
                for (int j = 0; j < 256; ++j) {
                    unsigned char len = 0;
                    auto it = code_lens.find((unsigned char)j);
                    if (it != code_lens.end()) len = (unsigned char)it->second;
                    outbuf.push_back(len);
                }
                for (size_t b = 0; b < sizeof(crc); ++b) {
                    outbuf.push_back((crc >> (8 * b)) & 0xFF);
                }
                outbuf.insert(outbuf.end(), buf.begin(), buf.end());
                std::lock_guard<std::mutex> lock(mtx);
                compressedChunks[i] = std::move(outbuf);
                chunkSizes[i] = compressedChunks[i].size();
            }));
        }
        for (auto& f : futures) f.get();

        // Write all chunks to output file
        std::ofstream out(outPath, std::ios::binary);
    if (!out) throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        // Main header: magic + numChunks + chunkSizes[]
        out.write("HUF_PAR", 7);
        uint32_t nChunks = static_cast<uint32_t>(numChunks);
        out.write(reinterpret_cast<const char*>(&nChunks), sizeof(nChunks));
        for (size_t i = 0; i < numChunks; ++i) {
            uint32_t sz = static_cast<uint32_t>(chunkSizes[i]);
            out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
        }
        // Write all compressed chunks
        for (const auto& buf : compressedChunks) {
            out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
        }
    if (out.bad()) throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        return true;
    } catch (const huffman::HuffmanError& e) {
        std::cerr << "Parallel compression error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error during parallel compression: " << e.what() << std::endl;
        return false;
    }
}
#include "../include/Compressor.h"
#include "../include/HuffmanTree.h"
#include "../include/BitWriter.h"
#include "../include/ErrorHandler.h"
#include "../include/Checksum.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Compressor::compress(const std::string& inPath, const std::string& outPath) {
    return compress(inPath, outPath, huffman::make_settings_from_level(5));
}

bool Compressor::compress(const std::string& inPath, const std::string& outPath, const huffman::CompressionSettings& settings) {
    return compressInternal(inPath, outPath, settings);
}

bool Compressor::compressInternal(const std::string& inPath, const std::string& outPath, const huffman::CompressionSettings& settings) {
    try {
        constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1MB
        std::ifstream in(inPath, std::ios::binary);
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_NOT_FOUND, inPath);
        }

        // First pass: count frequencies in chunks
        std::unordered_map<unsigned char, uint64_t> freq;
        size_t total = 0;
        std::vector<unsigned char> buffer(CHUNK_SIZE);
        while (in) {
            in.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
            std::streamsize bytesRead = in.gcount();
            if (bytesRead <= 0) break;
            total += static_cast<size_t>(bytesRead);
            for (std::streamsize i = 0; i < bytesRead; ++i) freq[buffer[i]]++;
        }
        in.clear();
        in.seekg(0, std::ios::beg);

        if (total == 0) {
            std::ofstream out(outPath, std::ios::binary);
            if (!out) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            out.write("HUF1", 4);
            uint16_t table_size = 0;
            out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
            return true;
        }

        if (settings.verbose) {
            std::cout << "Compressing with level " << settings.level
                      << " (mode: " << (settings.mode == huffman::CompressionSettings::FAST ? "FAST" :
                                        settings.mode == huffman::CompressionSettings::DEFAULT ? "DEFAULT" : "BEST")
                      << ")" << std::endl;
            std::cout << "Input size: " << total << " bytes" << std::endl;
            std::cout << "Unique symbols: " << freq.size() << std::endl;
        }

        // Build Huffman tree and code table
        HuffmanTree tree;
        tree.build(freq);
        auto canonical_codes = tree.getCanonicalCodes();

        // Write to output file
        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }

        // Write header: magic + code lengths for all 256 symbols
        out.write("HUF2", 4);
        HuffmanTree::CodeLenTable code_lens = tree.getCodeLengths();
        for (int i = 0; i < 256; ++i) {
            unsigned char len = 0;
            auto it = code_lens.find((unsigned char)i);
            if (it != code_lens.end()) len = (unsigned char)it->second;
            out.put(len);
        }

        // Second pass: encode data in chunks
        in.clear();
        in.seekg(0, std::ios::beg);
        BitWriter writer;
        size_t processed = 0;
        while (in) {
            in.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
            std::streamsize bytesRead = in.gcount();
            if (bytesRead <= 0) break;
            for (std::streamsize i = 0; i < bytesRead; ++i) {
                const std::string& bits = canonical_codes[buffer[i]];
                for (char b : bits) writer.writeBit(b == '1');
            }
            processed += static_cast<size_t>(bytesRead);
            if (settings.progress) {
                int percent = static_cast<int>((processed) * 100 / total);
                std::cout << "\rCompressing: " << percent << "% [";
                int bar = percent / 2;
                for (int j = 0; j < 50; ++j) std::cout << (j < bar ? '=' : ' ');
                std::cout << "] " << processed << "/" << total << std::flush;
                if (processed == total) std::cout << std::endl;
            }
        }
        writer.flush();
        const auto& buf = writer.getBuffer();

        // Calculate CRC32 of compressed data
        uint32_t crc = huffman::CRC32::calculate(buf);
        out.write(reinterpret_cast<const char*>(&crc), sizeof(crc));

        // Write compressed data
        out.write(reinterpret_cast<const char*>(buf.data()), buf.size());

        if (out.bad()) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }

        return true;
    } catch (const huffman::HuffmanError& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        switch (e.getCode()) {
            case huffman::ErrorCode::FILE_NOT_FOUND:
                std::cerr << "  Suggestion: Check the input file path and ensure the file exists." << std::endl;
                break;
            case huffman::ErrorCode::FILE_READ_ERROR:
            case huffman::ErrorCode::FILE_WRITE_ERROR:
                std::cerr << "  Suggestion: Check file permissions and disk space." << std::endl;
                break;
            case huffman::ErrorCode::INVALID_MAGIC:
            case huffman::ErrorCode::CORRUPTED_HEADER:
                std::cerr << "  Suggestion: The file may not be a valid Huffman-compressed file or is corrupted." << std::endl;
                break;
            case huffman::ErrorCode::COMPRESSION_FAILED:
                std::cerr << "  Suggestion: Try running with verbose mode for more details." << std::endl;
                break;
            case huffman::ErrorCode::INVALID_INPUT:
                std::cerr << "  Suggestion: Check input arguments and file format." << std::endl;
                break;
            case huffman::ErrorCode::MEMORY_ERROR:
                std::cerr << "  Suggestion: Not enough memory. Try smaller files or close other applications." << std::endl;
                break;
            default:
                break;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error during compression: " << e.what() << std::endl;
        std::cerr << "  Suggestion: Try running with verbose mode or check your input files." << std::endl;
        return false;
    }
}
