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

        // Read magic (up to 8 bytes)
        std::streampos start_pos = in.tellg();
        char magic[8] = {0};
        in.read(magic, 8);
        std::streamsize magic_read = in.gcount();
        if (magic_read < 4) {
            throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read magic number");
        }
        std::string magic_str(magic, static_cast<size_t>(magic_read));

        // Handle parallel container format: HUF_PAR
        if (magic_str.rfind("HUF_PAR", 0) == 0) {
            // Seek stream back so subsequent reads start just after the 7-byte magic
            if (magic_read > 7) in.seekg(static_cast<std::streamoff>(7 - magic_read), std::ios::cur);
            // Read number of chunks
            uint32_t nChunks = 0;
            in.read(reinterpret_cast<char*>(&nChunks), sizeof(nChunks));
            if (!in) throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read chunk count");

            std::vector<uint32_t> chunkSizes(nChunks);
            for (uint32_t i = 0; i < nChunks; ++i) {
                in.read(reinterpret_cast<char*>(&chunkSizes[i]), sizeof(chunkSizes[i]));
                if (!in) throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Cannot read chunk size");
            }

            // Process each chunk independently
            std::vector<unsigned char> final_out;
            for (uint32_t ci = 0; ci < nChunks; ++ci) {
                uint32_t sz = chunkSizes[ci];
                std::vector<unsigned char> chunkBuf(sz);
                in.read(reinterpret_cast<char*>(chunkBuf.data()), sz);
                std::streamsize got = in.gcount();
                if (static_cast<uint32_t>(got) != sz) {
                    throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Chunk truncated");
                }

                // Each chunk is itself a small HUF2-style blob: magic(4) + 256 code lengths + crc32 + compressed data
                if (sz < 4 + 256 + 4) {
                    throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Chunk too small");
                }
                std::string chunk_magic(reinterpret_cast<char*>(chunkBuf.data()), 4);
                if (chunk_magic != "HUF2") {
                    throw huffman::HuffmanError(huffman::ErrorCode::INVALID_MAGIC, chunk_magic);
                }

                size_t pos = 4;
                // Read original uncompressed size (uint64_t little-endian) if present (newer chunk format).
                uint64_t orig_uncompressed = 0;
                bool has_orig_size = false;
                if (chunkBuf.size() >= pos + sizeof(uint64_t) + 256 + 4) {
                    has_orig_size = true;
                    for (size_t b = 0; b < sizeof(orig_uncompressed); ++b) {
                        orig_uncompressed |= (uint64_t)chunkBuf[pos++] << (8 * b);
                    }
                } else if (chunkBuf.size() >= pos + 256 + 4) {
                    // Older chunk format without orig_size: proceed with code lengths at pos=4
                    has_orig_size = false;
                    orig_uncompressed = 0;
                } else {
                    throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "Chunk too small");
                }
                std::unordered_map<unsigned char, int> code_lens;
                for (int i = 0; i < 256; ++i) {
                    unsigned char len = chunkBuf[pos++];
                    if (len > 0) code_lens[(unsigned char)i] = len;
                }

                uint32_t crc_stored = 0;
                for (size_t b = 0; b < sizeof(crc_stored); ++b) {
                    crc_stored |= (uint32_t)chunkBuf[pos++] << (8 * b);
                }

                std::vector<unsigned char> crc_buf;
                if (pos < chunkBuf.size()) crc_buf.insert(crc_buf.end(), chunkBuf.begin() + pos, chunkBuf.end());
                if (crc_buf.empty()) {
                    throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "No compressed data in chunk");
                }

                uint32_t crc_calc = huffman::CRC32::calculate(crc_buf);
                if (crc_calc != crc_stored) {
                    throw huffman::HuffmanError(huffman::ErrorCode::CORRUPTED_HEADER, "CRC32 mismatch in chunk: file may be corrupted");
                }

                // Reconstruct canonical codes for this chunk
                std::vector<std::pair<unsigned char, int>> sorted;
                for (const auto& kv : code_lens) sorted.push_back(kv);
                std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
                    if (a.second != b.second) return a.second < b.second;
                    return a.first < b.first;
                });
                std::unordered_map<unsigned char, std::string> codes;
                if (!sorted.empty()) {
                    unsigned int code = 0;
                    int prev_len = sorted.front().second;
                    for (size_t i = 0; i < sorted.size(); ++i) {
                        int len = sorted[i].second;
                        if (i > 0) {
                            ++code;
                            if (len > prev_len) code <<= (len - prev_len);
                        }
                        prev_len = len;
                        codes[sorted[i].first] = bitstring(code, len);
                    }
                }
                std::unordered_map<std::string, unsigned char> rev_codes;
                for (const auto& kv : codes) rev_codes[kv.second] = kv.first;

                // Decode Huffman for this chunk
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
                                // If we have reached the expected uncompressed size, stop decoding
                                if (orig_uncompressed > 0 && huff_decoded.size() >= orig_uncompressed) {
                                    break;
                                }
                                break;
                            }
                        }
                        if (orig_uncompressed > 0 && huff_decoded.size() >= orig_uncompressed) break;
                    }
                }

                // Append decoded bytes for this chunk
                final_out.insert(final_out.end(), huff_decoded.begin(), huff_decoded.end());
            }

            // Write combined output file
            std::ofstream out(outPath, std::ios::binary);
            if (!out) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            out.write(reinterpret_cast<const char*>(final_out.data()), final_out.size());
            if (out.bad()) {
                throw huffman::HuffmanError(huffman::ErrorCode::FILE_WRITE_ERROR, outPath);
            }
            return true;
        }

        bool is_hybrid = false;
        if (magic_str.rfind("HUF_LZ77", 0) == 0) {
            is_hybrid = true;
        } else if (magic_str.substr(0,4) == "HUF2" || magic_str.substr(0,4) == "HUF1") {
            // legacy Huffman
        } else {
            throw huffman::HuffmanError(huffman::ErrorCode::INVALID_MAGIC, magic_str);
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
        if (!sorted.empty()) {
            unsigned int code = 0;
            int prev_len = sorted.front().second;
            for (size_t i = 0; i < sorted.size(); ++i) {
                int len = sorted[i].second;
                if (i > 0) {
                    ++code;
                    if (len > prev_len) code <<= (len - prev_len);
                }
                prev_len = len;
                codes[sorted[i].first] = bitstring(code, len);
            }
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
