#include "../include/FolderCompressor.h"
#include "../include/ErrorHandler.h"
#include "../include/Compressor.h"
#include "../include/Decompressor.h"
#include "../include/Checksum.h"
#include "../include/HuffmanCompressor.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace huffman {

FolderCompressor::FolderCompressor() : progress_callback_(nullptr) {}

FolderCompressor::~FolderCompressor() {}

void FolderCompressor::setProgressCallback(ProgressCallback callback) {
    progress_callback_ = std::move(callback);
}

std::vector<std::string> FolderCompressor::collectFiles(const std::string& folder_path) {
    std::vector<std::string> files;
    
    if (!fs::exists(folder_path)) {
        throw HuffmanError(ErrorCode::FILE_NOT_FOUND, "Folder not found: " + folder_path);
    }
    
    if (!fs::is_directory(folder_path)) {
        throw HuffmanError(ErrorCode::INVALID_INPUT, "Path is not a directory: " + folder_path);
    }
    
    // Recursively collect all regular files
    for (const auto& entry : fs::recursive_directory_iterator(folder_path)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    
    // Sort files for consistent ordering
    std::sort(files.begin(), files.end());
    
    return files;
}

std::string FolderCompressor::makeRelativePath(const std::string& base_path, 
                                                const std::string& full_path) {
    fs::path base(base_path);
    fs::path full(full_path);
    
    // Get relative path
    fs::path relative = fs::relative(full, base.parent_path());
    
    // Convert to forward slashes for cross-platform compatibility
    std::string rel_str = relative.string();
    std::replace(rel_str.begin(), rel_str.end(), '\\', '/');
    
    return rel_str;
}

void FolderCompressor::createDirectoryRecursive(const std::string& path) {
    fs::path p(path);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
}

uint32_t FolderCompressor::calculateCRC32(const std::vector<uint8_t>& data) {
    return CRC32::calculate(data);
}

bool FolderCompressor::writeArchiveHeader(std::ofstream& out, 
                                         const ArchiveMetadata& metadata) {
    // Write fixed header
    out.write(reinterpret_cast<const char*>(&metadata.header.magic), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&metadata.header.version), sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(&metadata.header.file_count), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&metadata.header.total_original_size), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&metadata.header.total_compressed_size), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&metadata.header.header_size), sizeof(uint64_t));
    
    // Write file entries
    for (const auto& file : metadata.files) {
        // Write path length and path string
        uint64_t path_len = file.relative_path.size();
        out.write(reinterpret_cast<const char*>(&path_len), sizeof(uint64_t));
        out.write(file.relative_path.c_str(), path_len);
        
        // Write file metadata
        out.write(reinterpret_cast<const char*>(&file.original_size), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&file.compressed_size), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&file.data_offset), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&file.timestamp), sizeof(uint64_t));
        out.write(reinterpret_cast<const char*>(&file.checksum), sizeof(uint32_t));
        
        // Write compression flag
        uint8_t compressed_flag = file.is_compressed ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&compressed_flag), sizeof(uint8_t));
    }
    
    return out.good();
}

bool FolderCompressor::readArchiveHeader(std::ifstream& in, 
                                        ArchiveMetadata& metadata) {
    // Read fixed header
    in.read(reinterpret_cast<char*>(&metadata.header.magic), sizeof(uint32_t));
    if (metadata.header.magic != ARCHIVE_MAGIC) {
        throw HuffmanError(ErrorCode::INVALID_MAGIC, "Invalid archive magic number");
    }
    
    in.read(reinterpret_cast<char*>(&metadata.header.version), sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(&metadata.header.file_count), sizeof(uint32_t));
    in.read(reinterpret_cast<char*>(&metadata.header.total_original_size), sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&metadata.header.total_compressed_size), sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&metadata.header.header_size), sizeof(uint64_t));
    
    // Read file entries
    metadata.files.clear();
    metadata.files.reserve(metadata.header.file_count);
    
    for (uint32_t i = 0; i < metadata.header.file_count; ++i) {
        FileEntry entry;
        
        // Read path
        uint64_t path_len;
        in.read(reinterpret_cast<char*>(&path_len), sizeof(uint64_t));
        std::vector<char> path_buf(path_len + 1, 0);
        in.read(path_buf.data(), path_len);
        entry.relative_path = std::string(path_buf.data(), path_len);
        
        // Read metadata
        in.read(reinterpret_cast<char*>(&entry.original_size), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.compressed_size), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.data_offset), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.timestamp), sizeof(uint64_t));
        in.read(reinterpret_cast<char*>(&entry.checksum), sizeof(uint32_t));
        
        // Read compression flag
        uint8_t compressed_flag;
        in.read(reinterpret_cast<char*>(&compressed_flag), sizeof(uint8_t));
        entry.is_compressed = (compressed_flag != 0);
        
        metadata.files.push_back(entry);
    }
    
    return in.good();
}

bool FolderCompressor::compressSingleFileToArchive(const std::string& file_path,
                                                   std::ofstream& archive_stream,
                                                   FileEntry& entry,
                                                   const CompressionSettings& settings) {
    // Read original file
    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        throw HuffmanError(ErrorCode::FILE_READ_ERROR, "Cannot read file: " + file_path);
    }
    
    std::vector<uint8_t> original_data((std::istreambuf_iterator<char>(input)),
                                       std::istreambuf_iterator<char>());
    input.close();
    
    entry.original_size = original_data.size();
    entry.checksum = calculateCRC32(original_data);
    
    // Get file timestamp
    auto ftime = fs::last_write_time(file_path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    entry.timestamp = std::chrono::system_clock::to_time_t(sctp);
    
    // Try to compress data
    std::vector<uint8_t> compressed_data = compressBuffer(original_data, settings);
    
    // Use smart compression: store if compressed is larger or similar size
    // Add 10% threshold - only compress if we save at least 10%
    bool should_compress = !compressed_data.empty() && 
                          (compressed_data.size() < original_data.size() * 0.9);
    
    if (should_compress) {
        // Use compressed data
        entry.is_compressed = true;
        entry.compressed_size = compressed_data.size();
        entry.data_offset = archive_stream.tellp();
        
        archive_stream.write(reinterpret_cast<const char*>(compressed_data.data()), 
                            compressed_data.size());
    } else {
        // Store uncompressed (better than expanding the file!)
        entry.is_compressed = false;
        entry.compressed_size = original_data.size();
        entry.data_offset = archive_stream.tellp();
        
        archive_stream.write(reinterpret_cast<const char*>(original_data.data()), 
                            original_data.size());
    }
    
    return archive_stream.good();
}

bool FolderCompressor::decompressSingleFileFromArchive(std::ifstream& archive_stream,
                                                       const FileEntry& entry,
                                                       const std::string& output_path) {
    // Seek to data position
    archive_stream.seekg(entry.data_offset);
    
    // Read data (compressed or stored)
    std::vector<uint8_t> file_data(entry.compressed_size);
    archive_stream.read(reinterpret_cast<char*>(file_data.data()), 
                       entry.compressed_size);
    
    if (!archive_stream.good()) {
        throw HuffmanError(ErrorCode::FILE_READ_ERROR, "Failed to read data");
    }
    
    std::vector<uint8_t> decompressed_data;
    
    if (entry.is_compressed) {
        // Decompress using Huffman decompressor buffer API
        decompressed_data = decompressBuffer(file_data);
        
        if (decompressed_data.empty()) {
            throw HuffmanError(ErrorCode::DECOMPRESSION_FAILED, 
                              "Failed to decompress: " + entry.relative_path);
        }
    } else {
        // File was stored uncompressed
        decompressed_data = std::move(file_data);
    }
    
    
    createDirectoryRecursive(output_path);
    
    // Write decompressed data
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        throw HuffmanError(ErrorCode::FILE_WRITE_ERROR, 
                          "Cannot write output file: " + output_path);
    }
    
    output.write(reinterpret_cast<const char*>(decompressed_data.data()), 
                decompressed_data.size());
    
    return output.good();
}

bool FolderCompressor::compressFolder(const std::string& folder_path,
                                     const std::string& archive_path,
                                     const CompressionSettings& settings) {
    try {
        // Collect all files in the folder
        std::vector<std::string> files = collectFiles(folder_path);
        
        if (files.empty()) {
            throw HuffmanError(ErrorCode::INVALID_INPUT, 
                              "No files found in folder: " + folder_path);
        }
        
        // Prepare archive metadata
        ArchiveMetadata metadata;
        metadata.header.file_count = files.size();
        metadata.files.reserve(files.size());
        
        // Create temporary file entries
        for (const auto& file : files) {
            FileEntry entry;
            entry.relative_path = makeRelativePath(folder_path, file);
            metadata.files.push_back(entry);
        }
        
        // Calculate header size (will be updated after compression)
        metadata.header.header_size = metadata.calculateHeaderSize();
        
        // Open archive file for writing
        std::ofstream archive(archive_path, std::ios::binary);
        if (!archive) {
            throw HuffmanError(ErrorCode::FILE_WRITE_ERROR, 
                              "Cannot create archive: " + archive_path);
        }
        
        // Reserve space for header (we'll write it at the end)
        std::vector<char> header_placeholder(metadata.header.header_size, 0);
        archive.write(header_placeholder.data(), header_placeholder.size());
        
        // Compress each file
        uint64_t total_original = 0;
        uint64_t total_compressed = 0;
        
        for (size_t i = 0; i < files.size(); ++i) {
            if (progress_callback_) {
                progress_callback_(i, files.size(), metadata.files[i].relative_path);
            }
            
            compressSingleFileToArchive(files[i], archive, metadata.files[i], settings);
            
            total_original += metadata.files[i].original_size;
            total_compressed += metadata.files[i].compressed_size;
        }
        
        // Update header with totals
        metadata.header.total_original_size = total_original;
        metadata.header.total_compressed_size = total_compressed;
        
        // Write header at the beginning
        archive.seekp(0);
        writeArchiveHeader(archive, metadata);
        
        archive.close();
        
        if (progress_callback_) {
            progress_callback_(files.size(), files.size(), "Complete");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (settings.verbose) {
            std::cerr << "Error compressing folder: " << e.what() << std::endl;
        }
        return false;
    }
}

bool FolderCompressor::decompressArchive(const std::string& archive_path,
                                        const std::string& output_folder) {
    try {
        // Open archive file
        std::ifstream archive(archive_path, std::ios::binary);
        if (!archive) {
            throw HuffmanError(ErrorCode::FILE_NOT_FOUND, 
                              "Archive not found: " + archive_path);
        }
        
        // Read archive metadata
        ArchiveMetadata metadata;
        readArchiveHeader(archive, metadata);
        
        // Create output folder
        fs::create_directories(output_folder);
        
        // Extract each file
        for (size_t i = 0; i < metadata.files.size(); ++i) {
            const auto& entry = metadata.files[i];
            
            if (progress_callback_) {
                progress_callback_(i, metadata.files.size(), entry.relative_path);
            }
            
            std::string output_path = (fs::path(output_folder) / entry.relative_path).string();
            decompressSingleFileFromArchive(archive, entry, output_path);
        }
        
        archive.close();
        
        if (progress_callback_) {
            progress_callback_(metadata.files.size(), metadata.files.size(), "Complete");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error decompressing archive: " << e.what() << std::endl;
        return false;
    }
}

ArchiveMetadata FolderCompressor::getArchiveInfo(const std::string& archive_path) {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
        throw HuffmanError(ErrorCode::FILE_NOT_FOUND, 
                          "Archive not found: " + archive_path);
    }
    
    ArchiveMetadata metadata;
    readArchiveHeader(archive, metadata);
    
    return metadata;
}

bool FolderCompressor::isValidArchive(const std::string& archive_path) {
    try {
        std::ifstream archive(archive_path, std::ios::binary);
        if (!archive) return false;
        
        uint32_t magic;
        archive.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
        
        return magic == ARCHIVE_MAGIC;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> FolderCompressor::listArchiveFiles(const std::string& archive_path) {
    ArchiveMetadata metadata = getArchiveInfo(archive_path);
    
    std::vector<std::string> file_list;
    file_list.reserve(metadata.files.size());
    
    for (const auto& entry : metadata.files) {
        file_list.push_back(entry.relative_path);
    }
    
    return file_list;
}

} // namespace huffman
