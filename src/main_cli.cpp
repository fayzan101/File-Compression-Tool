#include "../include/HuffmanTree.h"
#include "../include/Compressor.h"
 #include <unordered_map>
#include <fstream>
#include <sstream>
#include "../include/HuffmanCompressor.h"
#include "../include/CompressionSettings.h"
#include "../include/FolderCompressor.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <filesystem>

class HuffmanCLI {


public:
    struct Options {
        std::string command;
        std::string input_file;
        std::string output_file;
        int level = 5;
        bool verbose = false;
        bool progress = false;
        bool verify = false;
        bool benchmark = false;
        std::vector<std::string> benchmark_files;
    };

    // Simple INI parser for config file
    static void loadConfig(Options& opts) {
        std::ifstream cfg("config.ini");
        if (!cfg) return;
        std::string line, section;
        while (std::getline(cfg, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq), val = line.substr(eq + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            if (section == "defaults") {
                if (key == "level") opts.level = std::stoi(val);
                else if (key == "verbose") opts.verbose = (val == "true");
                else if (key == "progress") opts.progress = (val == "true");
                else if (key == "verify") opts.verify = (val == "true");
            }
        }
    }

    static void printUsage() {
        std::cout << "HuffmanCompressor v" << huffman::getVersion() << " - Advanced Huffman Compression Tool\n\n";
        std::cout << "=== INSTRUCTIONS ===" << std::endl;
        std::cout << "\n--- FILE COMPRESSION ---" << std::endl;
        std::cout << "Option 1: Compress File" << std::endl;
        std::cout << "  • Compresses a single file from the 'uploads' folder" << std::endl;
        std::cout << "  • Output saved to 'compressed' folder with .zip extension" << std::endl;
        std::cout << "  • Automatically chooses best method (parallel for large files)" << std::endl;
        std::cout << "  • Files that don't compress well are stored instead" << std::endl;
        
        std::cout << "\nOption 2: Hybrid Compress (LZ77 + Huffman)" << std::endl;
        std::cout << "  • Uses LZ77 (pattern matching) + Huffman (entropy coding)" << std::endl;
        std::cout << "  • Best for text files with repetitive content" << std::endl;
        std::cout << "  • Input from 'uploads', output to 'compressed'" << std::endl;
        
        std::cout << "\nOption 3: Decompress File" << std::endl;
        std::cout << "  • Decompresses a single .zip file from 'compressed' folder" << std::endl;
        std::cout << "  • Output saved to 'decompressed' folder" << std::endl;
        std::cout << "  • Automatically handles both compressed and stored files" << std::endl;
        
        std::cout << "\n--- FOLDER COMPRESSION ---" << std::endl;
        std::cout << "Option 4: Compress Folder" << std::endl;
        std::cout << "  • Compresses entire folder from 'uploads' directory" << std::endl;
        std::cout << "  • Creates archive in 'compressed' folder" << std::endl;
        std::cout << "  • Preserves folder structure and file metadata" << std::endl;
        std::cout << "  • Smart compression: stores files that don't compress well" << std::endl;
        
        std::cout << "\nOption 5: Decompress Archive" << std::endl;
        std::cout << "  • Extracts archive from 'compressed' folder" << std::endl;
        std::cout << "  • Restores to 'decompressed' folder with original structure" << std::endl;
        std::cout << "  • Verifies data integrity using CRC32 checksums" << std::endl;
        
        std::cout << "\nOption 6: List Archive Files" << std::endl;
        std::cout << "  • Shows contents of archive without extracting" << std::endl;
        std::cout << "  • Displays file sizes and compression ratios" << std::endl;
        std::cout << "  • Shows which files are compressed vs stored" << std::endl;
        
        std::cout << "\n--- ANALYSIS TOOLS ---" << std::endl;
        std::cout << "Option 7: Benchmark" << std::endl;
        std::cout << "  • Compares Huffman vs Gzip vs Bzip2 vs XZ" << std::endl;
        std::cout << "  • Tests compression ratio and speed" << std::endl;
        std::cout << "  • Helps choose best algorithm for your files" << std::endl;
        
        std::cout << "\nOption 8: Info" << std::endl;
        std::cout << "  • Shows detailed information about compressed files" << std::endl;
        std::cout << "  • Validates file format and integrity" << std::endl;
        
        std::cout << "\n--- COMPRESSION LEVELS ---" << std::endl;
        std::cout << "  1-3: Fast compression (less compression, faster speed)" << std::endl;
        std::cout << "  4-6: Default (balanced compression and speed)" << std::endl;
        std::cout << "  7-9: Best compression (maximum compression, slower)" << std::endl;
        
        std::cout << "\n--- FOLDER STRUCTURE ---" << std::endl;
        std::cout << "  uploads/       → Place files/folders to compress here" << std::endl;
        std::cout << "  compressed/    → Compressed .zip files stored here" << std::endl;
        std::cout << "  decompressed/  → Extracted files/folders appear here" << std::endl;
        
        std::cout << "\n--- FILE FORMATS ---" << std::endl;
        std::cout << "  .zip → Compressed files (Huffman or stored format)" << std::endl;
        std::cout << "  Archive format uses magic 'HFAR' for folder archives" << std::endl;
        std::cout << "  Stored files use magic 'STOR' when compression doesn't help" << std::endl;
        
        std::cout << "\n--- TIPS ---" << std::endl;
        std::cout << "  • Text files compress well (50-70% reduction typical)" << std::endl;
        std::cout << "  • Already compressed files (jpg, png, mp4) won't compress" << std::endl;
        std::cout << "  • Very small files (<100 bytes) are automatically stored" << std::endl;
        std::cout << "  • Use compression level 9 for maximum compression" << std::endl;
        std::cout << "  • Use hybrid mode for files with repetitive patterns" << std::endl;
        std::cout << "\n";
    }

    static Options parseArguments(int argc, char* argv[]) {
    Options opts;
    loadConfig(opts);
        
        if (argc < 2) {
            throw std::invalid_argument("No command specified");
        }

        opts.command = argv[1];

        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                printUsage();
                exit(0);
            } else if (arg == "--verbose" || arg == "-v") {
                opts.verbose = true;
            } else if (arg == "--progress" || arg == "-p") {
                opts.progress = true;
            } else if (arg == "--verify") {
                opts.verify = true;
            } else if (arg == "--level" || arg == "-l") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--level requires a value");
                }
                opts.level = std::stoi(argv[++i]);
                if (opts.level < 1 || opts.level > 9) {
                    throw std::invalid_argument("Level must be between 1 and 9");
                }
            } else if (arg == "--compare-gzip") {
                opts.benchmark = true;
            } else if (arg[0] != '-') {
                if (opts.input_file.empty()) {
                    opts.input_file = arg;
                } else if (opts.output_file.empty()) {
                    opts.output_file = arg;
                }
            }
        }

        return opts;
    }

    static void showProgress(size_t current, size_t total, const std::string& operation) {
        if (total == 0) return;
        
        int percent = (current * 100) / total;
        int bar_width = 50;
        int pos = (current * bar_width) / total;
        
        std::cout << "\r" << operation << ": " << percent << "% [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << current << "/" << total << std::flush;
        
        if (current == total) std::cout << std::endl;
    }

    static void compressFile(const Options& opts, bool parallel = false, bool hybrid = false) {
        if (opts.input_file.empty() || opts.output_file.empty()) {
            throw std::invalid_argument("Compress requires input and output files");
        }
        if (!std::filesystem::exists(opts.input_file)) {
            throw std::runtime_error("Input file does not exist: " + opts.input_file);
        }
        auto settings = huffman::make_settings_from_level(opts.level);
        settings.verbose = opts.verbose;
        if (opts.verbose) {
            std::cout << "Compressing: " << opts.input_file << " -> " << opts.output_file << std::endl;
            std::cout << "Level: " << opts.level << " (";
            if (opts.level <= 3) std::cout << "Fast";
            else if (opts.level <= 6) std::cout << "Default";
            else std::cout << "Best";
            std::cout << ")" << std::endl;
        }
        auto start = std::chrono::high_resolution_clock::now();
        bool success = false;
        if (hybrid) {
            // Hybrid compression
            Compressor compressor;
            success = compressor.compressInternal(opts.input_file, opts.output_file, settings); // uses hybrid
            if (success) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                size_t orig = std::filesystem::file_size(opts.input_file);
                size_t comp = huffman::getCompressedFileSize(opts.output_file);
                std::cout << "Compression successful!" << std::endl;
                std::cout << "Original size: " << orig << " bytes" << std::endl;
                std::cout << "Compressed size: " << comp << " bytes" << std::endl;
                std::cout << "Compression ratio: " << std::fixed << std::setprecision(1)
                          << (orig > 0 ? (double)comp / orig * 100.0 : 0.0) << "%" << std::endl;
                std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
            }
        } else if (parallel) {
            Compressor compressor;
            success = compressor.compressParallel(opts.input_file, opts.output_file, settings, 1024 * 1024); // 1MB chunks
            if (success) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                size_t orig = std::filesystem::file_size(opts.input_file);
                size_t comp = huffman::getCompressedFileSize(opts.output_file);
                std::cout << "Compression successful!" << std::endl;
                std::cout << "Original size: " << orig << " bytes" << std::endl;
                std::cout << "Compressed size: " << comp << " bytes" << std::endl;
                std::cout << "Compression ratio: " << std::fixed << std::setprecision(1)
                          << (orig > 0 ? (double)comp / orig * 100.0 : 0.0) << "%" << std::endl;
                std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
            }
        } else {
            auto result = huffman::compressFile(opts.input_file, opts.output_file, settings);
            success = result.success;
            if (success) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                
                // Check if compression actually reduced size
                if (result.compressed_size >= result.original_size * 0.95) {
                    std::cout << "Warning: Compression provides minimal benefit!" << std::endl;
                    std::cout << "Creating stored archive instead..." << std::endl;
                    
                    // Create a simple stored format: [STORE][size][data]
                    std::ofstream out(opts.output_file, std::ios::binary);
                    std::ifstream in(opts.input_file, std::ios::binary);
                    
                    // Write magic for stored file
                    out.write("STOR", 4);
                    
                    // Write original size
                    uint64_t size = result.original_size;
                    out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
                    
                    // Copy file data
                    out << in.rdbuf();
                    
                    in.close();
                    out.close();
                    
                    std::cout << "File stored (not compressed)" << std::endl;
                    std::cout << "Original size: " << result.original_size << " bytes" << std::endl;
                    std::cout << "Stored size: " << (result.original_size + 12) << " bytes (with header)" << std::endl;
                    std::cout << "Compression ratio: 100.0%" << std::endl;
                } else {
                    std::cout << "Compression successful!" << std::endl;
                    std::cout << "Original size: " << result.original_size << " bytes" << std::endl;
                    std::cout << "Compressed size: " << result.compressed_size << " bytes" << std::endl;
                    std::cout << "Compression ratio: " << std::fixed << std::setprecision(1) 
                              << result.compression_ratio << "%" << std::endl;
                }
                std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
            }
        }
        if (!success) {
            throw std::runtime_error("Compression failed");
        }
    }

    static void decompressFile(const Options& opts) {
        if (opts.input_file.empty() || opts.output_file.empty()) {
            throw std::invalid_argument("Decompress requires input and output files");
        }

        if (!std::filesystem::exists(opts.input_file)) {
            throw std::runtime_error("Input file does not exist: " + opts.input_file);
        }
        
        // Check if file is stored (not compressed)
        std::ifstream check(opts.input_file, std::ios::binary);
        char magic[4];
        check.read(magic, 4);
        check.close();
        
        if (std::string(magic, 4) == "STOR") {
            // Handle stored file
            if (opts.verbose) {
                std::cout << "Extracting stored file: " << opts.input_file << " -> " << opts.output_file << std::endl;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::ifstream in(opts.input_file, std::ios::binary);
            in.seekg(4); // Skip magic
            
            uint64_t size;
            in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
            
            std::ofstream out(opts.output_file, std::ios::binary);
            out << in.rdbuf();
            
            in.close();
            out.close();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            std::cout << "Extraction successful!" << std::endl;
            std::cout << "File size: " << size << " bytes" << std::endl;
            std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
            return;
        }

        // Handle compressed file
        if (opts.verbose) {
            std::cout << "Decompressing: " << opts.input_file << " -> " << opts.output_file << std::endl;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto result = huffman::decompressFile(opts.input_file, opts.output_file);
        auto end = std::chrono::high_resolution_clock::now();

        if (!result.success) {
            throw std::runtime_error("Decompression failed: " + result.error);
        }

        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Decompression successful!" << std::endl;
        std::cout << "Compressed size: " << result.compressed_size << " bytes" << std::endl;
        std::cout << "Decompressed size: " << result.original_size << " bytes" << std::endl;
        std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
    }

    static void showFileInfo(const Options& opts) {
        if (opts.input_file.empty()) {
            throw std::invalid_argument("Info requires a file");
        }

        if (!std::filesystem::exists(opts.input_file)) {
            throw std::runtime_error("File does not exist: " + opts.input_file);
        }

        std::cout << "File Information: " << opts.input_file << std::endl;
        std::cout << "Valid Huffman file: " << (huffman::isValidCompressedFile(opts.input_file) ? "Yes" : "No") << std::endl;
        
        if (huffman::isValidCompressedFile(opts.input_file)) {
            size_t size = huffman::getCompressedFileSize(opts.input_file);
            std::cout << "Compressed size: " << size << " bytes" << std::endl;
        }
    }

    static void runBenchmark(const Options& opts) {
        std::vector<std::string> files = opts.benchmark_files;
        if (files.empty()) {
            std::cout << "Enter files to benchmark (comma separated): ";
            std::string line; std::getline(std::cin, line);
            std::istringstream iss(line);
            std::string f;
            while (std::getline(iss, f, ',')) {
                if (!f.empty()) files.push_back(f);
            }
        }
        // Ask whether to show verbose/progress for Huffman compression during benchmark
        std::cout << "Show Huffman verbose output? (y/n, default n): ";
        std::string verboseStr; std::getline(std::cin, verboseStr);
        bool showVerbose = (!verboseStr.empty() && (verboseStr[0] == 'y' || verboseStr[0] == 'Y'));
        std::cout << "Show Huffman progress bar? (y/n, default n): ";
        std::string progressStr; std::getline(std::cin, progressStr);
        bool showProgress = (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y'));
        std::cout << "Enable parallel Huffman compression? (y/n, default n): ";
        std::string parallelStr; std::getline(std::cin, parallelStr);
        bool useParallel = (!parallelStr.empty() && (parallelStr[0] == 'y' || parallelStr[0] == 'Y'));
        std::cout << "Compression level for Huffman (1-9, default 5): ";
        std::string levelStr; std::getline(std::cin, levelStr);
        int benchLevel = 5;
        if (!levelStr.empty()) {
            try { benchLevel = std::stoi(levelStr); }
            catch(...) { benchLevel = 5; }
            if (benchLevel < 1) benchLevel = 1;
            if (benchLevel > 9) benchLevel = 9;
        }
        std::cout << "\nBenchmarking files: ";
        for (const auto& f : files) std::cout << f << " ";
        std::cout << std::endl;
        std::cout << std::left << std::setw(20) << "File"
                  << std::setw(12) << "Orig (KB)"
                  << std::setw(12) << "Huff (KB)"
                  << std::setw(12) << "Gzip (KB)"
                  << std::setw(12) << "Bzip2 (KB)"
                  << std::setw(12) << "XZ (KB)"
                  << std::setw(10) << "Huff(ms)"
                  << std::setw(10) << "Gzip(ms)"
                  << std::setw(10) << "Bzip2(ms)"
                  << std::setw(10) << "XZ(ms)"
                  << std::endl;
        for (const auto& file : files) {
            size_t orig_size = std::filesystem::file_size(file);
            std::string huf_out = file + ".huf";
            std::string gz_out = file + ".gz";
            std::string bz2_out = file + ".bz2";
            std::string xz_out = file + ".xz";
            // Huffman
            auto huf_start = std::chrono::high_resolution_clock::now();
            // Build options for Huffman compression based on benchmark prompts
            Options huffOpts;
            huffOpts.command = "compress";
            huffOpts.input_file = file;
            huffOpts.output_file = huf_out;
            huffOpts.level = benchLevel;
            huffOpts.verbose = showVerbose;
            huffOpts.progress = showProgress;
            compressFile(huffOpts, useParallel);
            auto huf_end = std::chrono::high_resolution_clock::now();
            size_t huf_size = std::filesystem::file_size(huf_out);
            double huf_time = std::chrono::duration<double, std::milli>(huf_end - huf_start).count();
            // gzip
            auto gz_start = std::chrono::high_resolution_clock::now();
            std::string gz_cmd = "gzip -kf \"" + file + "\"";
            system(gz_cmd.c_str());
            auto gz_end = std::chrono::high_resolution_clock::now();
            size_t gz_size = std::filesystem::file_size(gz_out);
            double gz_time = std::chrono::duration<double, std::milli>(gz_end - gz_start).count();
            // bzip2
            auto bz2_start = std::chrono::high_resolution_clock::now();
            std::string bz2_cmd = "bzip2 -kf \"" + file + "\"";
            system(bz2_cmd.c_str());
            auto bz2_end = std::chrono::high_resolution_clock::now();
            size_t bz2_size = std::filesystem::file_size(bz2_out);
            double bz2_time = std::chrono::duration<double, std::milli>(bz2_end - bz2_start).count();
            // xz
            auto xz_start = std::chrono::high_resolution_clock::now();
            std::string xz_cmd = "xz -kf \"" + file + "\"";
            system(xz_cmd.c_str());
            auto xz_end = std::chrono::high_resolution_clock::now();
            size_t xz_size = std::filesystem::file_size(xz_out);
            double xz_time = std::chrono::duration<double, std::milli>(xz_end - xz_start).count();
            std::cout << std::left << std::setw(20) << file
                      << std::setw(12) << (orig_size / 1024)
                      << std::setw(12) << (huf_size / 1024)
                      << std::setw(12) << (gz_size / 1024)
                      << std::setw(12) << (bz2_size / 1024)
                      << std::setw(12) << (xz_size / 1024)
                      << std::setw(10) << (int)huf_time
                      << std::setw(10) << (int)gz_time
                      << std::setw(10) << (int)bz2_time
                      << std::setw(10) << (int)xz_time
                      << std::endl;
        }
        std::cout << "\nBenchmark complete.\n";
    }
};

int main(int argc, char* argv[]) {
    std::cout << "\nWelcome to HuffmanCompressor!" << std::endl;
    while (true) {
        std::cout << "\nMenu:\n";
        std::cout << "  1. Compress File\n";
        std::cout << "  2. Hybrid Compress (LZ77 + Huffman)\n";
        std::cout << "  3. Decompress file\n";
        std::cout << "  4. Compress Folder\n";
        std::cout << "  5. Decompress Archive\n";
        std::cout << "  6. List Archive Files\n";
        std::cout << "  7. Benchmark\n";
        std::cout << "  8. Info (show compressed file info)\n";
        std::cout << "  9. Help\n";
        std::cout << "  0. Exit\n";
        std::cout << "Select an option: ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "0" || choice == "exit" || choice == "quit") break;
        try {
            if (choice == "1") {
                std::string inName, outName, levelStr, progressStr;
                int level = 5;
                bool verbose = true, progress = false;
                
                // List available files in uploads folder
                std::cout << "\nAvailable files in uploads folder:" << std::endl;
                int fileCount = 0;
                for (const auto& entry : std::filesystem::directory_iterator("uploads")) {
                    if (entry.is_regular_file()) {
                        std::cout << "  - " << entry.path().filename().string() << std::endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    std::cout << "  (No files found)" << std::endl;
                }
                std::cout << std::endl;
                
                std::cout << "Enter input file name: "; std::getline(std::cin, inName);
                std::string inPath = "uploads/" + inName;
                
                std::cout << "Enter output file name (without extension): "; std::getline(std::cin, outName);
                
                // Automatically add .zip extension if not present
                if (outName.find('.') == std::string::npos) {
                    outName += ".zip";
                }
                std::string outPath = "compressed/" + outName;
                std::cout << "Compression level (1-9, default 5): "; std::getline(std::cin, levelStr);
                if (!levelStr.empty()) level = std::stoi(levelStr);
                std::cout << "Show progress bar? (y/n): "; std::getline(std::cin, progressStr);
                if (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y')) progress = true;
                
                // Use smart compression with automatic parallel/regular selection
                size_t fileSize = std::filesystem::file_size(inPath);
                bool useParallel = (fileSize > 1024 * 1024); // Use parallel only for files > 1MB
                
                HuffmanCLI::compressFile({"compress", inPath, outPath, level, verbose, progress}, useParallel);
            } else if (choice == "2") {
                std::string inName, outName, levelStr, progressStr;
                int level = 5;
                bool verbose = true, progress = false;
                std::cout << "Enter input file name: "; std::getline(std::cin, inName);
                std::string inPath = "uploads/" + inName;
                
                std::cout << "Enter output file name (without extension): "; std::getline(std::cin, outName);
                
                // Automatically add .zip extension if not present
                if (outName.find('.') == std::string::npos) {
                    outName += ".zip";
                }
                std::string outPath = "compressed/" + outName;
                std::cout << "Compression level (1-9, default 5): "; std::getline(std::cin, levelStr);
                if (!levelStr.empty()) level = std::stoi(levelStr);
                std::cout << "Show progress bar? (y/n): "; std::getline(std::cin, progressStr);
                if (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y')) progress = true;
                HuffmanCLI::compressFile({"compress", inPath, outPath, level, verbose, progress}, false, true);
            } else if (choice == "3") {
                std::string inName, outName, verifyStr, progressStr;
                bool verify = false, progress = false;
                
                // List available files in compressed folder
                std::cout << "\nAvailable files in compressed folder:" << std::endl;
                int fileCount = 0;
                for (const auto& entry : std::filesystem::directory_iterator("compressed")) {
                    if (entry.is_regular_file()) {
                        std::cout << "  - " << entry.path().filename().string() << std::endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    std::cout << "  (No files found)" << std::endl;
                }
                std::cout << std::endl;
                
                std::cout << "Enter compressed file name (without extension): "; std::getline(std::cin, inName);
                
                // Automatically add .zip extension and use compressed folder
                if (inName.find('.') == std::string::npos) {
                    inName += ".zip";
                }
                std::string inPath = "compressed/" + inName;
                
                std::cout << "Enter output file name: "; std::getline(std::cin, outName);
                std::string outPath = "decompressed/" + outName;
                std::cout << "Verify data integrity? (y/n): "; std::getline(std::cin, verifyStr);
                if (!verifyStr.empty() && (verifyStr[0] == 'y' || verifyStr[0] == 'Y')) verify = true;
                std::cout << "Show progress bar? (y/n): "; std::getline(std::cin, progressStr);
                if (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y')) progress = true;
                HuffmanCLI::decompressFile({"decompress", inPath, outPath, 5, false, progress, verify});
            } else if (choice == "4") {
                // Compress folder
                std::string folderName, archiveName, levelStr, verboseStr;
                int level = 5;
                bool verbose = true;
                
                std::cout << "Enter folder name to compress (in uploads): "; 
                std::getline(std::cin, folderName);
                std::string folderPath = "uploads/" + folderName;
                
                std::cout << "Enter output archive name (without extension): "; 
                std::getline(std::cin, archiveName);
                
                // Automatically add .zip extension if not present
                if (archiveName.find('.') == std::string::npos) {
                    archiveName += ".zip";
                }
                
                // Save to compressed folder
                std::string archivePath = "compressed/" + archiveName;
                std::cout << "Compression level (1-9, default 5): "; 
                std::getline(std::cin, levelStr);
                if (!levelStr.empty()) level = std::stoi(levelStr);
                
                auto settings = huffman::make_settings_from_level(level);
                settings.verbose = verbose;
                
                huffman::FolderCompressor compressor;
                
                // Set progress callback
                compressor.setProgressCallback([](size_t current, size_t total, const std::string& file) {
                    std::cout << "\rCompressing: [" << (current + 1) << "/" << total << "] "
                              << file << "          " << std::flush;
                    if (current + 1 == total) std::cout << std::endl;
                });
                
                auto start = std::chrono::high_resolution_clock::now();
                bool success = compressor.compressFolder(folderPath, archivePath, settings);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (success) {
                    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                    auto info = compressor.getArchiveInfo(archivePath);
                    
                    std::cout << "\nFolder compression successful!" << std::endl;
                    std::cout << "Files compressed: " << info.header.file_count << std::endl;
                    std::cout << "Total original size: " << info.header.total_original_size << " bytes" << std::endl;
                    std::cout << "Total compressed size: " << info.header.total_compressed_size << " bytes" << std::endl;
                    std::cout << "Compression ratio: " << std::fixed << std::setprecision(1)
                              << (info.header.total_original_size > 0 
                                  ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                                  : 0.0) << "%" << std::endl;
                    std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
                } else {
                    std::cout << "Folder compression failed!" << std::endl;
                }
                
            } else if (choice == "5") {
                // Decompress archive
                std::string archiveName, outputFolderName;
                
                std::cout << "Enter archive name (without extension): "; 
                std::getline(std::cin, archiveName);
                
                // Automatically add .zip extension and use compressed folder
                if (archiveName.find('.') == std::string::npos) {
                    archiveName += ".zip";
                }
                std::string archivePath = "compressed/" + archiveName;
                
                std::cout << "Enter output folder name: "; 
                std::getline(std::cin, outputFolderName);
                std::string outputFolder = "decompressed/" + outputFolderName;
                
                huffman::FolderCompressor compressor;
                
                // Set progress callback
                compressor.setProgressCallback([](size_t current, size_t total, const std::string& file) {
                    std::cout << "\rExtracting: [" << (current + 1) << "/" << total << "] "
                              << file << "          " << std::flush;
                    if (current + 1 == total) std::cout << std::endl;
                });
                
                auto start = std::chrono::high_resolution_clock::now();
                bool success = compressor.decompressArchive(archivePath, outputFolder);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (success) {
                    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                    std::cout << "\nArchive extraction successful!" << std::endl;
                    std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
                } else {
                    std::cout << "Archive extraction failed!" << std::endl;
                }
                
            } else if (choice == "6") {
                // List archive files
                std::string archiveName;
                
                std::cout << "Enter archive name (without extension): "; 
                std::getline(std::cin, archiveName);
                
                // Automatically add .zip extension and use compressed folder
                if (archiveName.find('.') == std::string::npos) {
                    archiveName += ".zip";
                }
                std::string archivePath = "compressed/" + archiveName;
                
                huffman::FolderCompressor compressor;
                
                if (!compressor.isValidArchive(archivePath)) {
                    std::cout << "Not a valid Huffman folder archive!" << std::endl;
                } else {
                    auto info = compressor.getArchiveInfo(archivePath);
                    
                    std::cout << "\nArchive Information:" << std::endl;
                    std::cout << "Files: " << info.header.file_count << std::endl;
                    std::cout << "Total original size: " << info.header.total_original_size << " bytes" << std::endl;
                    std::cout << "Total compressed size: " << info.header.total_compressed_size << " bytes" << std::endl;
                    std::cout << "Compression ratio: " << std::fixed << std::setprecision(1)
                              << (info.header.total_original_size > 0 
                                  ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                                  : 0.0) << "%" << std::endl;
                    std::cout << "\nFile List:" << std::endl;
                    
                    size_t stored_count = 0;
                    size_t compressed_count = 0;
                    
                    for (size_t i = 0; i < info.files.size(); ++i) {
                        const auto& file = info.files[i];
                        std::cout << "  " << (i + 1) << ". " << file.relative_path 
                                  << " (" << file.original_size << " -> " << file.compressed_size << " bytes)";
                        
                        if (!file.is_compressed) {
                            std::cout << " [STORED]";
                            stored_count++;
                        } else {
                            compressed_count++;
                        }
                        std::cout << std::endl;
                    }
                    
                    std::cout << "\nCompression Summary:" << std::endl;
                    std::cout << "  Compressed files: " << compressed_count << std::endl;
                    std::cout << "  Stored files: " << stored_count << std::endl;
                }
            } else if (choice == "7") {
                HuffmanCLI::runBenchmark({"benchmark"});
            } else if (choice == "8") {
                std::string inPath;
                std::cout << "Enter compressed file path: "; 
                std::getline(std::cin, inPath);
                HuffmanCLI::showFileInfo({"info", inPath});
            } else if (choice == "9") {
                HuffmanCLI::printUsage();
            } else {
                std::cout << "Invalid option. Please enter a number from 0 to 9." << std::endl;
            }
        } catch (const huffman::HuffmanError& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            switch (e.getCode()) {
                case huffman::ErrorCode::FILE_NOT_FOUND:
                    std::cerr << "  Suggestion: Check the file path and ensure the file exists." << std::endl;
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
                case huffman::ErrorCode::COMPRESSION_FAILED:
                    std::cerr << "  Suggestion: Try running with --verbose for more details." << std::endl;
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
        } catch (const std::exception& e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
            std::cerr << "  Suggestion: Try running with --verbose or check your input files." << std::endl;
        }
    }
    std::cout << "Exiting HuffmanCompressor. Goodbye!" << std::endl;
    return 0;
}

