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

using namespace std;

class HuffmanCLI {
public:
    struct Options {
        string command;
        string input_file;
        string output_file;
        int level = 5;
        bool verbose = false;
        bool progress = false;
        bool verify = false;
        // ...existing code...
    };

    // Simple INI parser for config file
    static void loadConfig(Options& opts) {
        ifstream cfg("config.ini");
        if (!cfg) return;
        string line, section;
        while (getline(cfg, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }
            auto eq = line.find('=');
            if (eq == string::npos) continue;
            string key = line.substr(0, eq), val = line.substr(eq + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            if (section == "defaults") {
                if (key == "level") opts.level = stoi(val);
                else if (key == "verbose") opts.verbose = (val == "true");
                else if (key == "progress") opts.progress = (val == "true");
                else if (key == "verify") opts.verify = (val == "true");
            }
        }
    }

    static void printUsage() {
        cout << "HuffmanCompressor v" << huffman::getVersion() << " - Advanced Huffman Compression Tool\n\n";
        cout << "=== INSTRUCTIONS ===" << endl;
        cout << "\n--- FILE COMPRESSION ---" << endl;
        cout << "Option 1: Compress File" << endl;
        cout << "  - Compresses a single file from the 'uploads' folder" << endl;
        cout << "  - Output saved to 'compressed' folder with .zip extension" << endl;
        cout << "  - Automatically chooses best method (parallel for large files)" << endl;
        cout << "  - Files that don't compress well are stored instead" << endl;
        
        cout << "\nOption 2: Hybrid Compress (LZ77 + Huffman)" << endl;
        cout << "  - Uses LZ77 (pattern matching) + Huffman (entropy coding)" << endl;
        cout << "  - Best for text files with repetitive content" << endl;
        cout << "  - Input from 'uploads', output to 'compressed'" << endl;
        
        cout << "\nOption 3: Decompress File" << endl;
        cout << "  - Decompresses a single .zip file from 'compressed' folder" << endl;
        cout << "  - Output saved to 'decompressed' folder" << endl;
        cout << "  - Automatically handles both compressed and stored files" << endl;
        
        cout << "\n--- FOLDER COMPRESSION ---" << endl;
        cout << "Option 4: Compress Folder" << endl;
        cout << "  - Compresses entire folder from 'uploads' directory" << endl;
        cout << "  - Creates archive in 'compressed' folder" << endl;
        cout << "  - Preserves folder structure and file metadata" << endl;
        cout << "  - Smart compression: stores files that don't compress well" << endl;
        
        cout << "\nOption 5: Decompress Archive" << endl;
        cout << "  - Extracts archive from 'compressed' folder" << endl;
        cout << "  - Restores to 'decompressed' folder with original structure" << endl;
        cout << "  - Verifies data integrity using CRC32 checksums" << endl;
        
        cout << "\nOption 6: List Archive Files" << endl;
        cout << "  - Shows contents of archive without extracting" << endl;
        cout << "  - Displays file sizes and compression ratios" << endl;
        cout << "  - Shows which files are compressed vs stored" << endl;
        
        cout << "\n--- ANALYSIS TOOLS ---" << endl;
        // ...existing code...
        
        cout << "\nOption 8: Info" << endl;
        cout << "  - Shows detailed information about compressed files" << endl;
        cout << "  - Validates file format and integrity" << endl;
        
        cout << "\n--- COMPRESSION LEVELS ---" << endl;
        cout << "  1-3: Fast compression (less compression, faster speed)" << endl;
        cout << "  4-6: Default (balanced compression and speed)" << endl;
        cout << "  7-9: Best compression (maximum compression, slower)" << endl;
        
        cout << "\n--- FOLDER STRUCTURE ---" << endl;
        cout << "  uploads/       -> Place files/folders to compress here" << endl;
        cout << "  compressed/    -> Compressed .zip files stored here" << endl;
        cout << "  decompressed/  -> Extracted files/folders appear here" << endl;
        
        cout << "\n--- FILE FORMATS ---" << endl;
        cout << "  .zip -> Compressed files (Huffman or stored format)" << endl;
        cout << "  Archive format uses magic 'HFAR' for folder archives" << endl;
        cout << "  Stored files use magic 'STOR' when compression doesn't help" << endl;
        
        cout << "\n--- TIPS ---" << endl;
        cout << "  - Text files compress well (50-70% reduction typical)" << endl;
        cout << "  - Already compressed files (jpg, png, mp4) won't compress" << endl;
        cout << "  - Very small files (<100 bytes) are automatically stored" << endl;
        cout << "  - Use compression level 9 for maximum compression" << endl;
        cout << "  - Use hybrid mode for files with repetitive patterns" << endl;
        cout << "\n";
    }

    static Options parseArguments(int argc, char* argv[]) {
    Options opts;
    loadConfig(opts);
        
        if (argc < 2) {
            throw invalid_argument("No command specified");
        }

        opts.command = argv[1];

        for (int i = 2; i < argc; ++i) {
            string arg = argv[i];
            
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
                    throw invalid_argument("--level requires a value");
                }
                opts.level = stoi(argv[++i]);
                if (opts.level < 1 || opts.level > 9) {
                    throw invalid_argument("Level must be between 1 and 9");
                }
            // ...existing code...
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

    static void showProgress(size_t current, size_t total, const string& operation) {
        if (total == 0) return;
        
        int percent = (current * 100) / total;
        int bar_width = 50;
        int pos = (current * bar_width) / total;
        
        cout << "\r" << operation << ": " << percent << "% [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) cout << "=";
            else if (i == pos) cout << ">";
            else cout << " ";
        }
        cout << "] " << current << "/" << total << flush;
        
        if (current == total) cout << endl;
    }

    static void compressFile(const Options& opts, bool parallel = false, bool hybrid = false) {
        if (opts.input_file.empty() || opts.output_file.empty()) {
            throw invalid_argument("Compress requires input and output files");
        }
        if (!filesystem::exists(opts.input_file)) {
            throw runtime_error("Input file does not exist: " + opts.input_file);
        }
        auto settings = huffman::make_settings_from_level(opts.level);
        settings.verbose = opts.verbose;
        if (opts.verbose) {
            cout << "Compressing: " << opts.input_file << " -> " << opts.output_file << endl;
            cout << "Level: " << opts.level << " (";
            if (opts.level <= 3) cout << "Fast";
            else if (opts.level <= 6) cout << "Default";
            else cout << "Best";
            cout << ")" << endl;
        }
        auto start = chrono::high_resolution_clock::now();
        bool success = false;
        if (hybrid) {
            // Hybrid compression
            Compressor compressor;
            success = compressor.compressInternal(opts.input_file, opts.output_file, settings); // uses hybrid
            if (success) {
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration<double, milli>(end - start).count();
                size_t orig = filesystem::file_size(opts.input_file);
                size_t comp = huffman::getCompressedFileSize(opts.output_file);
                cout << "Compression successful!" << endl;
                cout << "Original size: " << orig << " bytes (" << fixed << setprecision(2) << (orig/1024.0) << " KB)" << endl;
                cout << "Compressed size: " << comp << " bytes (" << fixed << setprecision(2) << (comp/1024.0) << " KB)" << endl;
                cout << "Compression ratio: " << fixed << setprecision(1)
                          << (orig > 0 ? (double)comp / orig * 100.0 : 0.0) << "%" << endl;
                cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
            }
        } else if (parallel) {
            Compressor compressor;
            success = compressor.compressParallel(opts.input_file, opts.output_file, settings, 1024 * 1024); // 1MB chunks
            if (success) {
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration<double, milli>(end - start).count();
                size_t orig = filesystem::file_size(opts.input_file);
                size_t comp = huffman::getCompressedFileSize(opts.output_file);
                cout << "Compression successful!" << endl;
                cout << "Original size: " << orig << " bytes (" << fixed << setprecision(2) << (orig/1024.0) << " KB)" << endl;
                cout << "Compressed size: " << comp << " bytes (" << fixed << setprecision(2) << (comp/1024.0) << " KB)" << endl;
                cout << "Compression ratio: " << fixed << setprecision(1)
                          << (orig > 0 ? (double)comp / orig * 100.0 : 0.0) << "%" << endl;
                cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
            }
        } else {
            auto result = huffman::compressFile(opts.input_file, opts.output_file, settings);
            success = result.success;
            if (success) {
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration<double, milli>(end - start).count();
                
                // Check if compression actually reduced size
                if (result.compressed_size >= result.original_size * 0.95) {
                    cout << "Warning: Compression provides minimal benefit!" << endl;
                    cout << "Creating stored archive instead..." << endl;
                    
                    // Create a simple stored format: [STORE][size][data]
                    ofstream out(opts.output_file, ios::binary);
                    ifstream in(opts.input_file, ios::binary);
                    
                    // Write magic for stored file
                    out.write("STOR", 4);
                    
                    // Write original size
                    uint64_t size = result.original_size;
                    out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
                    
                    // Copy file data
                    out << in.rdbuf();
                    
                    in.close();
                    out.close();
                    
                    cout << "File stored (not compressed)" << endl;
                    cout << "Original size: " << result.original_size << " bytes (" << fixed << setprecision(2) << (result.original_size/1024.0) << " KB)" << endl;
                    cout << "Stored size: " << (result.original_size + 12) << " bytes (" << fixed << setprecision(2) << ((result.original_size+12)/1024.0) << " KB, with header)" << endl;
                    cout << "Compression ratio: 100.0%" << endl;
                } else {
                    cout << "Compression successful!" << endl;
                    cout << "Original size: " << result.original_size << " bytes (" << fixed << setprecision(2) << (result.original_size/1024.0) << " KB)" << endl;
                    cout << "Compressed size: " << result.compressed_size << " bytes (" << fixed << setprecision(2) << (result.compressed_size/1024.0) << " KB)" << endl;
                    cout << "Compression ratio: " << fixed << setprecision(1) 
                              << result.compression_ratio << "%" << endl;
                }
                cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
            }
        }
        if (!success) {
            throw runtime_error("Compression failed");
        }
    }

    static void decompressFile(const Options& opts) {
        if (opts.input_file.empty() || opts.output_file.empty()) {
            throw invalid_argument("Decompress requires input and output files");
        }

        if (!filesystem::exists(opts.input_file)) {
            throw runtime_error("Input file does not exist: " + opts.input_file);
        }
        
        // Check if file is stored (not compressed)
        ifstream check(opts.input_file, ios::binary);
        char magic[4];
        check.read(magic, 4);
        check.close();
        
        if (string(magic, 4) == "STOR") {
            // Handle stored file
            if (opts.verbose) {
                cout << "Extracting stored file: " << opts.input_file << " -> " << opts.output_file << endl;
            }
            
            auto start = chrono::high_resolution_clock::now();
            
            ifstream in(opts.input_file, ios::binary);
            in.seekg(4); // Skip magic
            
            uint64_t size;
            in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
            
            ofstream out(opts.output_file, ios::binary);
            out << in.rdbuf();
            
            in.close();
            out.close();
            
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration<double, milli>(end - start).count();
            
            cout << "Extraction successful!" << endl;
            cout << "File size: " << size << " bytes" << endl;
            cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
            return;
        }

        // Handle compressed file
        if (opts.verbose) {
            cout << "Decompressing: " << opts.input_file << " -> " << opts.output_file << endl;
        }

        auto start = chrono::high_resolution_clock::now();
        auto result = huffman::decompressFile(opts.input_file, opts.output_file);
        auto end = chrono::high_resolution_clock::now();

        if (!result.success) {
            throw runtime_error("Decompression failed: " + result.error);
        }

        auto duration = chrono::duration<double, milli>(end - start).count();
        
        cout << "Decompression successful!" << endl;
        cout << "Compressed size: " << result.compressed_size << " bytes (" << fixed << setprecision(2) << (result.compressed_size/1024.0) << " KB)" << endl;
        cout << "Decompressed size: " << result.original_size << " bytes (" << fixed << setprecision(2) << (result.original_size/1024.0) << " KB)" << endl;
        cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
    }

    static void showFileInfo(const Options& opts) {
        if (opts.input_file.empty()) {
            throw invalid_argument("Info requires a file");
        }

        if (!filesystem::exists(opts.input_file)) {
            throw runtime_error("File does not exist: " + opts.input_file);
        }

        cout << "File Information: " << opts.input_file << endl;
        cout << "Valid Huffman file: " << (huffman::isValidCompressedFile(opts.input_file) ? "Yes" : "No") << endl;
        
        if (huffman::isValidCompressedFile(opts.input_file)) {
            size_t size = huffman::getCompressedFileSize(opts.input_file);
            cout << "Compressed size: " << size << " bytes" << endl;
        }
    }

    // ...existing code...
};

int main(int argc, char* argv[]) {
    cout << "\nWelcome to HuffmanCompressor!" << endl;
    while (true) {
        cout << "\nMenu:\n";
        cout << "  1. Compress File\n";
        cout << "  2. Hybrid Compress (LZ77 + Huffman)\n";
        cout << "  3. Decompress file\n";
        cout << "  4. Compress Folder\n";
        cout << "  5. Decompress Folder\n";
        cout << "  6. List Archive Files\n";
        // ...existing code...
        cout << "  8. Info (show compressed file info)\n";
        cout << "  9. Show Huffman Tree (DOT export)\n";
        cout << " 10. Help\n";
        cout << "  0. Exit\n";
        cout << "Select an option: ";
        string choice;
        getline(cin, choice);
        if (choice == "0" || choice == "exit" || choice == "quit") break;
        try {
            if (choice == "1") {
                string inName, outName, levelStr;
                int level = 5;
                bool verbose = true, progress = true;
                
                // List available files in uploads folder
                cout << "\nAvailable files in uploads folder:" << endl;
                int fileCount = 0;
                for (const auto& entry : filesystem::directory_iterator("uploads")) {
                    if (entry.is_regular_file()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    cout << "  (No files found)" << endl;
                }
                cout << endl;
                
                cout << "Enter input file name: "; getline(cin, inName);
                string inPath = "uploads/" + inName;
                
                cout << "Enter output file name (without extension): "; getline(cin, outName);
                
                // Automatically add .zip extension if not present
                if (outName.find('.') == string::npos) {
                    outName += ".zip";
                }
                string outPath = "compressed/" + outName;
                cout << "Compression level (1-9, default 5): "; getline(cin, levelStr);
                if (!levelStr.empty()) level = stoi(levelStr);
                // Progress bar enabled by default
                
                // Use smart compression with automatic parallel/regular selection
                size_t fileSize = filesystem::file_size(inPath);
                bool useParallel = (fileSize > 1024 * 1024); // Use parallel only for files > 1MB
                
                HuffmanCLI::compressFile({"compress", inPath, outPath, level, verbose, progress}, useParallel);
            } else if (choice == "2") {
                string inName, outName, levelStr;
                int level = 5;
                bool verbose = true, progress = true;
                // List available files in uploads folder
                cout << "\nAvailable files in uploads folder:" << endl;
                int fileCount = 0;
                for (const auto& entry : filesystem::directory_iterator("uploads")) {
                    if (entry.is_regular_file()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    cout << "  (No files found)" << endl;
                }
                cout << endl;
                cout << "Enter input file name: "; getline(cin, inName);
                string inPath = "uploads/" + inName;
                cout << "Enter output file name (without extension): "; getline(cin, outName);
                // Automatically add .zip extension if not present
                if (outName.find('.') == string::npos) {
                    outName += ".zip";
                }
                string outPath = "compressed/" + outName;
                cout << "Compression level (1-9, default 5): "; getline(cin, levelStr);
                if (!levelStr.empty()) level = stoi(levelStr);
                // Progress bar enabled by default
                HuffmanCLI::compressFile({"compress", inPath, outPath, level, verbose, progress}, false, true);
            } else if (choice == "3") {
                string inName, outName, verifyStr;
                bool verify = false, progress = true;
                
                // List available files in compressed folder
                cout << "\nAvailable files in compressed folder:" << endl;
                int fileCount = 0;
                for (const auto& entry : filesystem::directory_iterator("compressed")) {
                    if (entry.is_regular_file()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    cout << "  (No files found)" << endl;
                }
                cout << endl;
                
                cout << "Enter compressed file name (without extension): "; getline(cin, inName);
                
                // Automatically add .zip extension and use compressed folder
                if (inName.find('.') == string::npos) {
                    inName += ".zip";
                }
                string inPath = "compressed/" + inName;
                
                cout << "Enter output file name: "; getline(cin, outName);
                string outPath = "decompressed/" + outName;
                // Data integrity verification enabled by default
                verify = true;
                // Progress bar enabled by default
                HuffmanCLI::decompressFile({"decompress", inPath, outPath, 5, false, progress, verify});
            } else if (choice == "4") {
                // Compress folder

                string folderName, archiveName, levelStr, verboseStr;
                int level = 5;
                bool verbose = true;

                // List available folders in uploads
                cout << "Available folders in uploads:" << endl;
                for (const auto& entry : filesystem::directory_iterator("uploads")) {
                    if (entry.is_directory()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                    }
                }
                cout << endl;
                cout << "Enter folder name to compress (in uploads): ";
                getline(cin, folderName);
                string folderPath = "uploads/" + folderName;
                
                cout << "Enter output archive name (without extension): "; 
                getline(cin, archiveName);
                
                // Automatically add .zip extension if not present
                if (archiveName.find('.') == string::npos) {
                    archiveName += ".zip";
                }
                
                // Save to compressed folder
                string archivePath = "compressed/" + archiveName;
                cout << "Compression level (1-9, default 5): "; 
                getline(cin, levelStr);
                if (!levelStr.empty()) level = stoi(levelStr);
                
                auto settings = huffman::make_settings_from_level(level);
                settings.verbose = verbose;

                // Prompt for hybrid mode
                

                huffman::FolderCompressor compressor;

                // Set progress callback
                compressor.setProgressCallback([](size_t current, size_t total, const string& file) {
                    cout << "\rCompressing: [" << (current + 1) << "/" << total << "] "
                              << file << "          " << flush;
                    if (current + 1 == total) cout << endl;
                });

                auto start = chrono::high_resolution_clock::now();
                bool success = compressor.compressFolder(folderPath, archivePath, settings);
                auto end = chrono::high_resolution_clock::now();

                if (success) {
                    auto duration = chrono::duration<double, milli>(end - start).count();
                    auto info = compressor.getArchiveInfo(archivePath);

                    cout << "\nFolder compression successful!" << endl;
                    cout << "Files compressed: " << info.header.file_count << endl;
                    cout << "Total original size: " << info.header.total_original_size << " bytes (" << fixed << setprecision(2) << (info.header.total_original_size/1024.0) << " KB)" << endl;
                    cout << "Total compressed size: " << info.header.total_compressed_size << " bytes (" << fixed << setprecision(2) << (info.header.total_compressed_size/1024.0) << " KB)" << endl;
                    cout << "Compression ratio: " << fixed << setprecision(1)
                              << (info.header.total_original_size > 0 
                                  ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                                  : 0.0) << "%" << endl;
                    cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
                } else {
                    cout << "Folder compression failed!" << endl;
                }
                
            } else if (choice == "5") {
                // Decompress archive
                string archiveName, outputFolderName;
                
                cout << "Enter archive name (without extension): "; 
                getline(cin, archiveName);
                
                // Automatically add .zip extension and use compressed folder
                if (archiveName.find('.') == string::npos) {
                    archiveName += ".zip";
                }
                string archivePath = "compressed/" + archiveName;
                
                cout << "Enter output folder name: "; 
                getline(cin, outputFolderName);
                string outputFolder = "decompressed/" + outputFolderName;
                
                huffman::FolderCompressor compressor;
                
                // Set progress callback
                compressor.setProgressCallback([](size_t current, size_t total, const string& file) {
                    cout << "\rExtracting: [" << (current + 1) << "/" << total << "] "
                              << file << "          " << flush;
                    if (current + 1 == total) cout << endl;
                });
                
                auto start = chrono::high_resolution_clock::now();
                bool success = compressor.decompressArchive(archivePath, outputFolder);
                auto end = chrono::high_resolution_clock::now();
                
                if (success) {
                    auto duration = chrono::duration<double, milli>(end - start).count();
                    cout << "\nArchive extraction successful!" << endl;
                    cout << "Time: " << fixed << setprecision(2) << duration << " ms" << endl;
                } else {
                    cout << "Archive extraction failed!" << endl;
                }
                
            } else if (choice == "6") {
                // List archive files
                string archiveName;
                
                cout << "Enter archive name (without extension): "; 
                getline(cin, archiveName);
                
                // Automatically add .zip extension and use compressed folder
                if (archiveName.find('.') == string::npos) {
                    archiveName += ".zip";
                }
                string archivePath = "compressed/" + archiveName;
                
                huffman::FolderCompressor compressor;
                
                if (!compressor.isValidArchive(archivePath)) {
                    cout << "Not a valid Huffman folder archive!" << endl;
                } else {
                    auto info = compressor.getArchiveInfo(archivePath);
                    
                    cout << "\nArchive Information:" << endl;
                    cout << "Files: " << info.header.file_count << endl;
                    cout << "Total original size: " << info.header.total_original_size << " bytes" << endl;
                    cout << "Total compressed size: " << info.header.total_compressed_size << " bytes" << endl;
                    cout << "Compression ratio: " << fixed << setprecision(1)
                              << (info.header.total_original_size > 0 
                                  ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                                  : 0.0) << "%" << endl;
                    cout << "\nFile List:" << endl;
                    
                    size_t stored_count = 0;
                    size_t compressed_count = 0;
                    
                    for (size_t i = 0; i < info.files.size(); ++i) {
                        const auto& file = info.files[i];
                        cout << "  " << (i + 1) << ". " << file.relative_path 
                                  << " (" << file.original_size << " -> " << file.compressed_size << " bytes)";
                        
                        if (!file.is_compressed) {
                            cout << " [STORED]";
                            stored_count++;
                        } else {
                            compressed_count++;
                        }
                        cout << endl;
                    }
                    
                    cout << "\nCompression Summary:" << endl;
                    cout << "  Compressed files: " << compressed_count << endl;
                    cout << "  Stored files: " << stored_count << endl;
                }
            // ...existing code...
            } else if (choice == "8") {
                // Show available files in compressed folder
                cout << "\nAvailable files in compressed folder:" << endl;
                int fileCount = 0;
                for (const auto& entry : filesystem::directory_iterator("compressed")) {
                    if (entry.is_regular_file()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    cout << "  (No files found)" << endl;
                }
                cout << endl;
                cout << "Enter compressed file name (without extension): ";
                string inName;
                getline(cin, inName);
                // Automatically add .zip extension and use compressed folder
                if (inName.find('.') == string::npos) {
                    inName += ".zip";
                }
                string inPath = "compressed/" + inName;
                HuffmanCLI::showFileInfo({"info", inPath});
            } else if (choice == "9") {
                // Show Huffman Tree (DOT export)
                // List available files in uploads folder
                cout << "\nAvailable files in uploads folder:" << endl;
                int fileCount = 0;
                for (const auto& entry : filesystem::directory_iterator("uploads")) {
                    if (entry.is_regular_file()) {
                        cout << "  - " << entry.path().filename().string() << endl;
                        fileCount++;
                    }
                }
                if (fileCount == 0) {
                    cout << "  (No files found)" << endl;
                }
                cout << endl;
                string inName;
                cout << "Enter input file name (from uploads): ";
                getline(cin, inName);
                string inPath = "uploads/" + inName;
                ifstream fin(inPath, ios::binary);
                if (!fin) {
                    cout << "File not found: " << inPath << endl;
                } else {
                    unordered_map<unsigned char, uint64_t> freq;
                    char c;
                    while (fin.get(c)) freq[(unsigned char)c]++;
                    if (freq.empty()) {
                        cout << "File is empty or unreadable." << endl;
                    } else {
                        HuffmanTree tree;
                        tree.build(freq);
                        string dot = tree.toDot();
                        ofstream fout("uploads/tree.dot");
                        fout << dot;
                        fout.close();
                        cout << "Huffman tree DOT file written to uploads/tree.dot\n";
                    }
                }
            } else if (choice == "10") {
                HuffmanCLI::printUsage();
            } else {
                cout << "Invalid option. Please enter a number from 0 to 9." << endl;
            }
        } catch (const huffman::HuffmanError& e) {
            cerr << "Error: " << e.what() << endl;
            switch (e.getCode()) {
                case huffman::ErrorCode::FILE_NOT_FOUND:
                    cerr << "  Suggestion: Check the file path and ensure the file exists." << endl;
                    break;
                case huffman::ErrorCode::FILE_READ_ERROR:
                case huffman::ErrorCode::FILE_WRITE_ERROR:
                    cerr << "  Suggestion: Check file permissions and disk space." << endl;
                    break;
                case huffman::ErrorCode::INVALID_MAGIC:
                case huffman::ErrorCode::CORRUPTED_HEADER:
                    cerr << "  Suggestion: The file may not be a valid Huffman-compressed file or is corrupted." << endl;
                    break;
                case huffman::ErrorCode::DECOMPRESSION_FAILED:
                case huffman::ErrorCode::COMPRESSION_FAILED:
                    cerr << "  Suggestion: Try running with --verbose for more details." << endl;
                    break;
                case huffman::ErrorCode::INVALID_INPUT:
                    cerr << "  Suggestion: Check input arguments and file format." << endl;
                    break;
                case huffman::ErrorCode::MEMORY_ERROR:
                    cerr << "  Suggestion: Not enough memory. Try smaller files or close other applications." << endl;
                    break;
                default:
                    break;
            }
        } catch (const exception& e) {
            cerr << "Unexpected error: " << e.what() << endl;
            cerr << "  Suggestion: Try running with --verbose or check your input files." << endl;
        }
    }
    cout << "Exiting HuffmanCompressor. Goodbye!" << endl;
    return 0;
}


