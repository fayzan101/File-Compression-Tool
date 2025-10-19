#include "../include/HuffmanTree.h"
#include "../include/BitReader.h"
#include "../include/ErrorHandler.h"
#include "../include/Checksum.h"
#include "../include/Decompressor.h"
#include <string>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "../include/LZ77.h"

// Helper: convert int to bitstring of given length
static std::string bitstring(int code, int len) {
    std::string s;
    for (int i = len - 1; i >= 0; --i) s += ((code >> i) & 1) ? '1' : '0';
    return s;
}
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool Decompressor::decompress(const std::string& inPath, const std::string& outPath) {
    try {
        // Read compressed file
        std::ifstream in(inPath, std::ios::binary);
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_NOT_FOUND, inPath);
        }

        // Read magic
        char magic[8] = {0};
        in.read(magic, 8);
        if (in.gcount() < 4) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read magic number");
        }
        bool is_hybrid = false;
        if (std::string(magic, 8).substr(0,8) == "HUF_LZ77") {
            is_hybrid = true;
        } else if (std::string(magic, 4) == "HUF2") {
            // legacy Huffman
        } else {
            throw huffman::HuffmanError(huffman::ErrorCode::INVALID_MAGIC, std::string(magic, 8));
        }

        // Read 256 code lengths
        std::unordered_map<unsigned char, int> code_lens;
        for (int i = 0; i < 256; ++i) {
            int len = in.get();
            if (len < 0) throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Unexpected end of file while reading code lengths");
            if (len > 0) code_lens[(unsigned char)i] = len;
        }

        // Handle empty file case
        if (code_lens.empty()) {
            std::ofstream out(outPath, std::ios::binary);
            if (!out) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            // Empty file - just create empty output
            return true;
        }

        // Read CRC32
        uint32_t crc_stored = 0;
        in.read(reinterpret_cast<char*>(&crc_stored), sizeof(crc_stored));
        if (!in) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read CRC32");
        }

        // Stream compressed data and decode in chunks
        constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1MB
        std::vector<unsigned char> buf;
        buf.reserve(CHUNK_SIZE);
        std::streampos data_start = in.tellg();
        in.seekg(0, std::ios::end);
        std::streampos data_end = in.tellg();
        size_t data_size = static_cast<size_t>(data_end - data_start);
        in.seekg(data_start, std::ios::beg);
        size_t bytes_left = data_size;

        // Read all compressed data for CRC check (streaming CRC possible, but keep logic simple)
        std::vector<unsigned char> crc_buf;
        while (bytes_left > 0) {
            size_t to_read = std::min(CHUNK_SIZE, bytes_left);
            std::vector<unsigned char> temp(to_read);
            in.read(reinterpret_cast<char*>(temp.data()), to_read);
            std::streamsize bytesRead = in.gcount();
            if (bytesRead <= 0) break;
            crc_buf.insert(crc_buf.end(), temp.begin(), temp.begin() + bytesRead);
            bytes_left -= static_cast<size_t>(bytesRead);
        }
        if (crc_buf.empty()) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "No compressed data found");
        }
        uint32_t crc_calc = huffman::CRC32::calculate(crc_buf);
        if (crc_calc != crc_stored) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "CRC32 mismatch: file may be corrupted");
        }

        // Reconstruct canonical codes
        std::vector<std::pair<unsigned char, int>> sorted;
        for (const auto& kv : code_lens) sorted.push_back(kv);
        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second < b.second;
            return a.first < b.first;
        });
        std::unordered_map<unsigned char, std::string> codes;
        int code = 0, prev_len = 0;
        for (const auto& kv : sorted) {
            int len = kv.second;
            if (len != prev_len) code <<= (len - prev_len);
            codes[kv.first] = bitstring(code, len);
            code++;
            prev_len = len;
        }
        std::unordered_map<std::string, unsigned char> rev_codes;
        for (const auto& kv : codes) rev_codes[kv.second] = kv.first;

        // Decode Huffman
        std::vector<unsigned char> huff_decoded;
        {
            BitReader reader(crc_buf);
            std::string cur;
            while (reader.hasMoreBits()) {
                cur.clear();
                while (true) {
                    if (!reader.hasMoreBits()) break;
                    bool bit = reader.readBit();
                    cur += bit ? '1' : '0';
                    auto it = rev_codes.find(cur);
                    if (it != rev_codes.end()) {
                        huff_decoded.push_back(it->second);
                        break;
                    }
                }
            }
        }

        // If hybrid, apply LZ77 decompression
        std::vector<unsigned char> final_out;
        if (is_hybrid) {
            auto tokens = LZ77::bytesToTokens(huff_decoded);
            final_out = LZ77::decompress(tokens);
        } else {
            final_out = std::move(huff_decoded);
        }

        // Write output file
        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }
        out.write(reinterpret_cast<const char*>(final_out.data()), final_out.size());
        if (out.bad()) {
            throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
        }
        return true;
    } catch (const huffman::HuffmanError& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
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
            case huffman::ErrorCode::DECOMPRESSION_FAILED:
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
        std::cerr << "Unexpected error during decompression: " << e.what() << std::endl;
        std::cerr << "  Suggestion: Try running with verbose mode or check your input files." << std::endl;
        return false;
    }
}
