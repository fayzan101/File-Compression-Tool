    #include "../include/HuffmanTree.h"
    #include "../include/Compressor.h"
    #include <unordered_map>
    #include <fstream>
    #include <sstream>
#include "../include/HuffmanCompressor.h"
#include "../include/CompressionSettings.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <filesystem>

class HuffmanCLI {

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

    static void printUsage() {
        std::cout << "HuffmanCompressor v" << huffman::getVersion() << " - Advanced Huffman Compression Tool\n\n";
        std::cout << "USAGE:\n";
        std::cout << "  huffman compress <input> <output> [options]\n";
        std::cout << "  huffman decompress <input> <output> [options]\n";
        std::cout << "  huffman info <compressed_file>\n";
        std::cout << "  huffman benchmark [options]\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "  --level N        Compression level (1-9, default: 5)\n";
        std::cout << "  --verbose        Show detailed information\n";
        std::cout << "  --progress       Show progress for large files\n";
        std::cout << "  --verify         Verify data integrity with checksums\n";
        std::cout << "  --help           Show this help message\n\n";
        std::cout << "EXAMPLES:\n";
        std::cout << "  huffman compress document.txt document.huf --level 9 --verbose\n";
        std::cout << "  huffman decompress document.huf restored.txt --verify\n";
        std::cout << "  huffman info document.huf\n";
        std::cout << "  huffman benchmark --compare-gzip\n";
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

    static void compressFile(const Options& opts, bool parallel = false) {
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
        if (parallel) {
            Compressor compressor;
            success = compressor.compressParallel(opts.input_file, opts.output_file, settings, 1024 * 1024); // 1MB chunks
        } else {
            auto result = huffman::compressFile(opts.input_file, opts.output_file, settings);
            success = result.success;
            if (success) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                std::cout << "Compression successful!" << std::endl;
                std::cout << "Original size: " << result.original_size << " bytes" << std::endl;
                std::cout << "Compressed size: " << result.compressed_size << " bytes" << std::endl;
                std::cout << "Compression ratio: " << std::fixed << std::setprecision(1) 
                          << result.compression_ratio << "%" << std::endl;
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
            compressFile({"compress", file, huf_out, 5, false, false});
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
        std::cout << "  1. Compress file\n";
        std::cout << "  2. Decompress file\n";
        std::cout << "  3. Info (show compressed file info)\n";
        std::cout << "  4. Benchmark\n";
        std::cout << "  5. Help\n";
    // ...existing code...
    std::cout << "  0. Exit\n";
        std::cout << "Select an option: ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "0" || choice == "exit" || choice == "quit") break;
        try {
            if (choice == "1") {
                std::string inPath, outPath, levelStr, verboseStr, progressStr, parallelStr;
                int level = 5;
                bool verbose = false, progress = false, parallel = false;
                std::cout << "Enter input file path: "; std::getline(std::cin, inPath);
                std::cout << "Enter output file path: "; std::getline(std::cin, outPath);
                std::cout << "Compression level (1-9, default 5): "; std::getline(std::cin, levelStr);
                if (!levelStr.empty()) level = std::stoi(levelStr);
                std::cout << "Show verbose output? (y/n): "; std::getline(std::cin, verboseStr);
                if (!verboseStr.empty() && (verboseStr[0] == 'y' || verboseStr[0] == 'Y')) verbose = true;
                std::cout << "Show progress bar? (y/n): "; std::getline(std::cin, progressStr);
                if (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y')) progress = true;
                std::cout << "Enable parallel compression? (y/n): "; std::getline(std::cin, parallelStr);
                if (!parallelStr.empty() && (parallelStr[0] == 'y' || parallelStr[0] == 'Y')) parallel = true;
                HuffmanCLI::compressFile({"compress", inPath, outPath, level, verbose, progress}, parallel);
            } else if (choice == "2") {
                std::string inPath, outPath, verifyStr, progressStr;
                bool verify = false, progress = false;
                std::cout << "Enter input file path: "; std::getline(std::cin, inPath);
                std::cout << "Enter output file path: "; std::getline(std::cin, outPath);
                std::cout << "Verify data integrity? (y/n): "; std::getline(std::cin, verifyStr);
                if (!verifyStr.empty() && (verifyStr[0] == 'y' || verifyStr[0] == 'Y')) verify = true;
                std::cout << "Show progress bar? (y/n): "; std::getline(std::cin, progressStr);
                if (!progressStr.empty() && (progressStr[0] == 'y' || progressStr[0] == 'Y')) progress = true;
                HuffmanCLI::decompressFile({"decompress", inPath, outPath, 5, false, progress, verify});
            } else if (choice == "3") {
                std::string inPath;
                std::cout << "Enter compressed file path: "; std::getline(std::cin, inPath);
                HuffmanCLI::showFileInfo({"info", inPath});
                // ...existing code...
            } else if (choice == "4") {
                HuffmanCLI::runBenchmark({"benchmark"});
            } else if (choice == "5") {
                HuffmanCLI::printUsage();
            } else {
                std::cout << "Invalid option. Please enter a number from 0 to 5." << std::endl;
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

