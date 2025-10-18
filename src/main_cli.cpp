#include "../include/HuffmanCompressor.h"
#include "../include/CompressionSettings.h"
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

    static void compressFile(const Options& opts) {
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
        auto result = huffman::compressFile(opts.input_file, opts.output_file, settings);
        auto end = std::chrono::high_resolution_clock::now();

        if (!result.success) {
            throw std::runtime_error("Compression failed: " + result.error);
        }

        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Compression successful!" << std::endl;
        std::cout << "Original size: " << result.original_size << " bytes" << std::endl;
        std::cout << "Compressed size: " << result.compressed_size << " bytes" << std::endl;
        std::cout << "Compression ratio: " << std::fixed << std::setprecision(1) 
                  << result.compression_ratio << "%" << std::endl;
        std::cout << "Time: " << std::fixed << std::setprecision(2) << duration << " ms" << std::endl;
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
        std::cout << "Running benchmark..." << std::endl;
        std::cout << "This feature will be implemented in the next phase." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            HuffmanCLI::printUsage();
            return 1;
        }

        auto opts = HuffmanCLI::parseArguments(argc, argv);

        if (opts.command == "compress") {
            HuffmanCLI::compressFile(opts);
        } else if (opts.command == "decompress") {
            HuffmanCLI::decompressFile(opts);
        } else if (opts.command == "info") {
            HuffmanCLI::showFileInfo(opts);
        } else if (opts.command == "benchmark") {
            HuffmanCLI::runBenchmark(opts);
        } else {
            std::cerr << "Unknown command: " << opts.command << std::endl;
            HuffmanCLI::printUsage();
            return 1;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

