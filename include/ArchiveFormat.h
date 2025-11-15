#ifndef ARCHIVE_FORMAT_H
#define ARCHIVE_FORMAT_H

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

namespace huffman {

// Archive file magic number "HFAR" (Huffman Folder ARchive)
const uint32_t ARCHIVE_MAGIC = 0x52414648;
const uint16_t ARCHIVE_VERSION = 1;

// Structure to store metadata for a single file in the archive
struct FileEntry {
    std::string relative_path;      // Relative path from archive root
    uint64_t original_size;         // Original file size in bytes
    uint64_t compressed_size;       // Compressed size in bytes
    uint64_t data_offset;           // Offset in archive where compressed data starts
    uint64_t timestamp;             // File modification timestamp
    uint32_t checksum;              // CRC32 checksum of original data
    bool is_compressed;             // True if compressed, false if stored
    
    FileEntry() : original_size(0), compressed_size(0), 
                  data_offset(0), timestamp(0), checksum(0), is_compressed(true) {}
};

// Archive header structure
struct ArchiveHeader {
    uint32_t magic;                 // Magic number for identification
    uint16_t version;               // Archive format version
    uint32_t file_count;            // Number of files in archive
    uint64_t total_original_size;   // Sum of all original file sizes
    uint64_t total_compressed_size; // Sum of all compressed file sizes
    uint64_t header_size;           // Size of the complete header (including file entries)
    
    ArchiveHeader() : magic(ARCHIVE_MAGIC), version(ARCHIVE_VERSION),
                      file_count(0), total_original_size(0),
                      total_compressed_size(0), header_size(0) {}
};

// Archive metadata - contains header and all file entries
struct ArchiveMetadata {
    ArchiveHeader header;
    std::vector<FileEntry> files;
    
    // Calculate total header size including all file entries
    uint64_t calculateHeaderSize() const {
        // Fixed header size + variable file entry data
        uint64_t size = sizeof(ArchiveHeader);
        for (const auto& file : files) {
            size += sizeof(uint64_t);  // path length
            size += file.relative_path.size();  // path string
            size += sizeof(uint64_t) * 4;  // original_size, compressed_size, data_offset, timestamp
            size += sizeof(uint32_t);  // checksum
            size += sizeof(uint8_t);   // is_compressed flag
        }
        return size;
    }
};

} // namespace huffman

#endif // ARCHIVE_FORMAT_H
