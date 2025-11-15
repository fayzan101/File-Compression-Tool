#ifndef FOLDER_COMPRESSOR_H
#define FOLDER_COMPRESSOR_H

#include "ArchiveFormat.h"
#include "CompressionSettings.h"
#include "HuffmanCompressor.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace huffman {

class FolderCompressor {
public:
    FolderCompressor();
    ~FolderCompressor();

    // Compress an entire folder into a single archive file
    // folder_path: Path to the folder to compress
    // archive_path: Output archive file path
    // settings: Compression settings to use
    // Returns: true if successful, false otherwise
    bool compressFolder(const std::string& folder_path, 
                       const std::string& archive_path,
                       const CompressionSettings& settings = CompressionSettings());

    // Decompress an archive file to a destination folder
    // archive_path: Path to the archive file
    // output_folder: Destination folder for extracted files
    // Returns: true if successful, false otherwise
    bool decompressArchive(const std::string& archive_path,
                          const std::string& output_folder);

    // Get information about an archive without extracting
    ArchiveMetadata getArchiveInfo(const std::string& archive_path);

    // Validate if a file is a valid Huffman folder archive
    bool isValidArchive(const std::string& archive_path);

    // Get list of files in an archive
    std::vector<std::string> listArchiveFiles(const std::string& archive_path);

    // Progress callback type: (current_file_index, total_files, current_file_name)
    using ProgressCallback = std::function<void(size_t, size_t, const std::string&)>;
    
    // Set progress callback for compression/decompression operations
    void setProgressCallback(ProgressCallback callback);

private:
    ProgressCallback progress_callback_;
    
    // Helper functions
    std::vector<std::string> collectFiles(const std::string& folder_path);
    std::string makeRelativePath(const std::string& base_path, const std::string& full_path);
    bool writeArchiveHeader(std::ofstream& out, const ArchiveMetadata& metadata);
    bool readArchiveHeader(std::ifstream& in, ArchiveMetadata& metadata);
    bool compressSingleFileToArchive(const std::string& file_path,
                                     std::ofstream& archive_stream,
                                     FileEntry& entry,
                                     const CompressionSettings& settings);
    bool decompressSingleFileFromArchive(std::ifstream& archive_stream,
                                        const FileEntry& entry,
                                        const std::string& output_path);
    void createDirectoryRecursive(const std::string& path);
    uint32_t calculateCRC32(const std::vector<uint8_t>& data);
};

} // namespace huffman

#endif // FOLDER_COMPRESSOR_H
