#include "../include/FolderCompressor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

namespace fs = std::filesystem;

void createTestFolder() {
    // Create test folder structure
    fs::create_directories("test_folder/subfolder1");
    fs::create_directories("test_folder/subfolder2");
    
    // Create test files
    std::ofstream f1("test_folder/file1.txt");
    f1 << "This is test file 1 with some content to compress.\n";
    f1 << "Multiple lines of text to ensure compression works.\n";
    f1.close();
    
    std::ofstream f2("test_folder/file2.txt");
    f2 << "Another test file with different content.\n";
    f2 << "Testing folder compression functionality.\n";
    f2.close();
    
    std::ofstream f3("test_folder/subfolder1/nested.txt");
    f3 << "This is a nested file in subfolder1.\n";
    f3 << "It should be preserved in the archive structure.\n";
    f3.close();
    
    std::ofstream f4("test_folder/subfolder2/data.txt");
    f4 << "Data file in subfolder2.\n";
    f4 << "Testing recursive directory compression.\n";
    f4.close();
}

void cleanupTestFolder() {
    if (fs::exists("test_folder")) {
        fs::remove_all("test_folder");
    }
    if (fs::exists("test_archive.hfa")) {
        fs::remove("test_archive.hfa");
    }
    if (fs::exists("extracted_folder")) {
        fs::remove_all("extracted_folder");
    }
}

bool compareFiles(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    
    if (!f1 || !f2) return false;
    
    std::vector<char> data1((std::istreambuf_iterator<char>(f1)),
                            std::istreambuf_iterator<char>());
    std::vector<char> data2((std::istreambuf_iterator<char>(f2)),
                            std::istreambuf_iterator<char>());
    
    return data1 == data2;
}

int main() {
    std::cout << "=== Folder Compression Test ===" << std::endl;
    
    try {
        // Cleanup from previous runs
        cleanupTestFolder();
        
        // Test 1: Create test folder structure
        std::cout << "\n[1] Creating test folder structure..." << std::endl;
        createTestFolder();
        std::cout << "    ✓ Test folder created" << std::endl;
        
        // Test 2: Compress folder
        std::cout << "\n[2] Compressing folder..." << std::endl;
        huffman::FolderCompressor compressor;
        
        // Set progress callback
        compressor.setProgressCallback([](size_t current, size_t total, const std::string& file) {
            std::cout << "    Compressing: [" << (current + 1) << "/" << total << "] " 
                      << file << std::endl;
        });
        
        huffman::CompressionSettings settings;
        settings.verbose = true;
        
        bool compress_success = compressor.compressFolder("test_folder", "test_archive.hfa", settings);
        assert(compress_success && "Folder compression failed");
        std::cout << "    ✓ Folder compressed successfully" << std::endl;
        
        // Test 3: Validate archive
        std::cout << "\n[3] Validating archive..." << std::endl;
        bool is_valid = compressor.isValidArchive("test_archive.hfa");
        assert(is_valid && "Archive validation failed");
        std::cout << "    ✓ Archive is valid" << std::endl;
        
        // Test 4: Get archive info
        std::cout << "\n[4] Reading archive information..." << std::endl;
        auto info = compressor.getArchiveInfo("test_archive.hfa");
        std::cout << "    Files in archive: " << info.header.file_count << std::endl;
        std::cout << "    Total original size: " << info.header.total_original_size << " bytes" << std::endl;
        std::cout << "    Total compressed size: " << info.header.total_compressed_size << " bytes" << std::endl;
        std::cout << "    Compression ratio: " << std::fixed << std::setprecision(1)
                  << (info.header.total_original_size > 0 
                      ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                      : 0.0) << "%" << std::endl;
        assert(info.header.file_count == 4 && "Expected 4 files in archive");
        std::cout << "    ✓ Archive info retrieved correctly" << std::endl;
        
        // Test 5: List files
        std::cout << "\n[5] Listing archive files..." << std::endl;
        auto file_list = compressor.listArchiveFiles("test_archive.hfa");
        std::cout << "    Files:" << std::endl;
        for (const auto& file : file_list) {
            std::cout << "      - " << file << std::endl;
        }
        assert(file_list.size() == 4 && "Expected 4 files in list");
        std::cout << "    ✓ File list retrieved correctly" << std::endl;
        
        // Test 6: Decompress archive
        std::cout << "\n[6] Decompressing archive..." << std::endl;
        compressor.setProgressCallback([](size_t current, size_t total, const std::string& file) {
            std::cout << "    Extracting: [" << (current + 1) << "/" << total << "] " 
                      << file << std::endl;
        });
        
        bool decompress_success = compressor.decompressArchive("test_archive.hfa", "extracted_folder");
        assert(decompress_success && "Archive decompression failed");
        std::cout << "    ✓ Archive decompressed successfully" << std::endl;
        
        // Test 7: Verify extracted files
        std::cout << "\n[7] Verifying extracted files..." << std::endl;
        bool files_match = true;
        
        files_match &= compareFiles("test_folder/file1.txt", "extracted_folder/test_folder/file1.txt");
        files_match &= compareFiles("test_folder/file2.txt", "extracted_folder/test_folder/file2.txt");
        files_match &= compareFiles("test_folder/subfolder1/nested.txt", 
                                   "extracted_folder/test_folder/subfolder1/nested.txt");
        files_match &= compareFiles("test_folder/subfolder2/data.txt", 
                                   "extracted_folder/test_folder/subfolder2/data.txt");
        
        assert(files_match && "Extracted files don't match originals");
        std::cout << "    ✓ All files match originals" << std::endl;
        
        // Test 8: Check folder structure
        std::cout << "\n[8] Verifying folder structure..." << std::endl;
        bool structure_ok = true;
        structure_ok &= fs::exists("extracted_folder/test_folder/file1.txt");
        structure_ok &= fs::exists("extracted_folder/test_folder/file2.txt");
        structure_ok &= fs::exists("extracted_folder/test_folder/subfolder1/nested.txt");
        structure_ok &= fs::exists("extracted_folder/test_folder/subfolder2/data.txt");
        
        assert(structure_ok && "Folder structure not preserved");
        std::cout << "    ✓ Folder structure preserved" << std::endl;
        
        // Cleanup
        std::cout << "\n[9] Cleaning up test files..." << std::endl;
        cleanupTestFolder();
        std::cout << "    ✓ Cleanup complete" << std::endl;
        
        std::cout << "\n=== ALL TESTS PASSED ✓ ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with error: " << e.what() << std::endl;
        cleanupTestFolder();
        return 1;
    }
}
