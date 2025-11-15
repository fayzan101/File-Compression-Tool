#pragma once

#include <string>
#include <stdexcept>

namespace huffman {

enum class ErrorCode {
    SUCCESS = 0,
    FILE_NOT_FOUND,
    FILE_READ_ERROR,
    FILE_WRITE_ERROR,
    INVALID_MAGIC,
    CORRUPTED_HEADER,
    DECOMPRESSION_FAILED,
    COMPRESSION_FAILED,
    INVALID_INPUT,
    MEMORY_ERROR,
    CHECKSUM_MISMATCH
};

class HuffmanError : public std::runtime_error {
public:
    explicit HuffmanError(ErrorCode code, const std::string& message = "")
        : std::runtime_error(getErrorMessage(code, message)), code_(code) {}
    
    ErrorCode getCode() const { return code_; }
    
private:
    ErrorCode code_;
    
    static std::string getErrorMessage(ErrorCode code, const std::string& message) {
        switch (code) {
            case ErrorCode::SUCCESS:
                return "Success";
            case ErrorCode::FILE_NOT_FOUND:
                return "File not found: " + message;
            case ErrorCode::FILE_READ_ERROR:
                return "Error reading file: " + message;
            case ErrorCode::FILE_WRITE_ERROR:
                return "Error writing file: " + message;
            case ErrorCode::INVALID_MAGIC:
                return "Invalid magic number in compressed file: " + message;
            case ErrorCode::CORRUPTED_HEADER:
                return "Corrupted header in compressed file: " + message;
            case ErrorCode::DECOMPRESSION_FAILED:
                return "Decompression failed: " + message;
            case ErrorCode::COMPRESSION_FAILED:
                return "Compression failed: " + message;
            case ErrorCode::INVALID_INPUT:
                return "Invalid input: " + message;
            case ErrorCode::MEMORY_ERROR:
                return "Memory allocation error: " + message;
            case ErrorCode::CHECKSUM_MISMATCH:
                return "Checksum mismatch (data corruption detected): " + message;
            default:
                return "Unknown error: " + message;
        }
    }
};

} // namespace huffman

