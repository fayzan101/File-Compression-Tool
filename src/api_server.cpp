    // ...existing code...
// Define Windows version for ASIO
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7 or later
#endif
#endif

#define ASIO_STANDALONE
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS

#include "../include/HuffmanTree.h"
#include "../include/HuffmanCompressor.h"
#include "../include/FolderCompressor.h"
#include "../include/CompressionSettings.h"
#include <crow.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

// Helper function to read file to string
std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to write string to file
void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
}

int main() {

    crow::SimpleApp app;
    // API endpoint: POST /api/tree-dot - Generate Huffman tree DOT file for a file in uploads
    CROW_ROUTE(app, "/api/tree-dot").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }
        std::string filename = body["filename"].s();
        std::string inputPath = "uploads/" + filename;
        if (!fs::exists(inputPath)) {
            crow::json::wvalue error;
            error["error"] = "File not found";
            error["path"] = inputPath;
            return crow::response(404, error);
        }
        std::ifstream fin(inputPath, std::ios::binary);
        if (!fin) {
            crow::json::wvalue error;
            error["error"] = "Failed to open file";
            error["path"] = inputPath;
            return crow::response(500, error);
        }
        std::unordered_map<unsigned char, uint64_t> freq;
        char c;
        while (fin.get(c)) freq[(unsigned char)c]++;
        if (freq.empty()) {
            crow::json::wvalue error;
            error["error"] = "File is empty or unreadable";
            return crow::response(400, error);
        }
        HuffmanTree tree;
        tree.build(freq);
        std::string dot = tree.toDot();
        // Ensure dot/ directory exists
        std::string dotDir = "dot";
        if (!fs::exists(dotDir)) {
            fs::create_directory(dotDir);
        }
        std::string dotPath = dotDir + "/" + filename + ".dot";
        writeFile(dotPath, dot);
        crow::json::wvalue response;
        response["success"] = true;
        response["dot_file"] = dotPath;
        response["dot_content"] = dot;
        return crow::response(200, response);
    });

    // Enable CORS
    app.loglevel(crow::LogLevel::Info);

    // API endpoint: GET / - Welcome message
    CROW_ROUTE(app, "/")
    ([]() {
        crow::json::wvalue response;
        response["message"] = "HuffmanCompressor API Server";
        response["version"] = huffman::getVersion();
        response["endpoints"] = crow::json::wvalue::list({
            "/api/compress - POST - Compress a file",
            "/api/decompress - POST - Decompress a file",
            "/api/compress-folder - POST - Compress a folder",
            "/api/decompress-folder - POST - Decompress an archive",
            "/api/list - GET - List files in uploads folder",
            "/api/info/<filename> - GET - Get compressed file info",
            "/api/tree-dot - POST - Generate Huffman tree DOT file for a file in uploads"
        });
        return crow::response(200, response);
    });

    // API endpoint: POST /api/compress - Compress a file
    CROW_ROUTE(app, "/api/compress").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string filename = body["filename"].s();
        int level = body.has("level") ? body["level"].i() : 5; // Default level 5
        
        std::string inputPath = "uploads/" + filename;
        std::string outputName = filename + ".zip";
        std::string outputPath = "compressed/" + outputName;

        // Check if file exists
        if (!fs::exists(inputPath)) {
            crow::json::wvalue error;
            error["error"] = "File not found";
            error["path"] = inputPath;
            return crow::response(404, error);
        }

        try {
            // Compress the file
            auto settings = huffman::make_settings_from_level(level);
            settings.verbose = false;
            
            auto start = std::chrono::high_resolution_clock::now();
            auto result = huffman::compressFile(inputPath, outputPath, settings);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();

            if (!result.success) {
                crow::json::wvalue error;
                error["error"] = "Compression failed";
                error["message"] = result.error;
                return crow::response(500, error);
            }

            // Check if stored instead
            bool stored = false;
            if (result.compressed_size >= result.original_size * 0.95) {
                stored = true;
                // Create stored format
                std::ofstream out(outputPath, std::ios::binary);
                std::ifstream in(inputPath, std::ios::binary);
                out.write("STOR", 4);
                uint64_t size = result.original_size;
                out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
                out << in.rdbuf();
            }

            crow::json::wvalue response;
            response["success"] = true;
            response["filename"] = filename;
            response["output"] = outputName;
            response["original_size"] = result.original_size;
            response["compressed_size"] = stored ? result.original_size + 12 : result.compressed_size;
            response["compression_ratio"] = stored ? 100.0 : result.compression_ratio;
            response["time_ms"] = duration;
            response["stored"] = stored;
            response["level"] = level;

            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Exception occurred";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // API endpoint: POST /api/decompress - Decompress a file
    CROW_ROUTE(app, "/api/decompress").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string filename = body["filename"].s();
        std::string outputName = body["output"].s();
        
        std::string inputPath = "compressed/" + filename;
        std::string outputPath = "decompressed/" + outputName;

        if (!fs::exists(inputPath)) {
            crow::json::wvalue error;
            error["error"] = "File not found";
            error["path"] = inputPath;
            return crow::response(404, error);
        }

        try {
            // Check if stored format
            std::ifstream check(inputPath, std::ios::binary);
            char magic[4];
            check.read(magic, 4);
            check.close();

            auto start = std::chrono::high_resolution_clock::now();
            
            bool was_stored = false;
            size_t file_size = 0;

            if (std::string(magic, 4) == "STOR") {
                // Extract stored file
                was_stored = true;
                std::ifstream in(inputPath, std::ios::binary);
                in.seekg(4);
                uint64_t size;
                in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
                file_size = size;
                
                std::ofstream out(outputPath, std::ios::binary);
                out << in.rdbuf();
            } else {
                // Decompress
                auto result = huffman::decompressFile(inputPath, outputPath);
                if (!result.success) {
                    crow::json::wvalue error;
                    error["error"] = "Decompression failed";
                    error["message"] = result.error;
                    return crow::response(500, error);
                }
                file_size = result.original_size;
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();

            crow::json::wvalue response;
            response["success"] = true;
            response["filename"] = filename;
            response["output"] = outputName;
            response["size"] = file_size;
            response["time_ms"] = duration;
            response["was_stored"] = was_stored;

            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Exception occurred";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // API endpoint: POST /api/compress-folder - Compress a folder
    CROW_ROUTE(app, "/api/compress-folder").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string foldername = body["folder"].s();
        std::string archiveName = std::string(body["archive"].s()) + ".zip";
        int level = body.has("level") ? body["level"].i() : 5;
        
        std::string folderPath = "uploads/" + foldername;
        std::string archivePath = "compressed/" + archiveName;

        if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
            crow::json::wvalue error;
            error["error"] = "Folder not found";
            error["path"] = folderPath;
            return crow::response(404, error);
        }

        try {
            auto settings = huffman::make_settings_from_level(level);
            settings.verbose = false;

            huffman::FolderCompressor compressor;
            
            auto start = std::chrono::high_resolution_clock::now();
            bool success = compressor.compressFolder(folderPath, archivePath, settings);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();

            if (!success) {
                crow::json::wvalue error;
                error["error"] = "Folder compression failed";
                return crow::response(500, error);
            }

            auto info = compressor.getArchiveInfo(archivePath);

            crow::json::wvalue response;
            response["success"] = true;
            response["folder"] = foldername;
            response["archive"] = archiveName;
            response["file_count"] = info.header.file_count;
            response["original_size"] = info.header.total_original_size;
            response["compressed_size"] = info.header.total_compressed_size;
            response["compression_ratio"] = (info.header.total_original_size > 0 
                ? (double)info.header.total_compressed_size / info.header.total_original_size * 100.0 
                : 0.0);
            response["time_ms"] = duration;

            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Exception occurred";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // API endpoint: POST /api/decompress-folder - Decompress archive
    CROW_ROUTE(app, "/api/decompress-folder").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string archiveName = std::string(body["archive"].s()) + ".zip";
        std::string outputFolder = body["output"].s();
        
        std::string archivePath = "compressed/" + archiveName;
        std::string outputPath = "decompressed/" + outputFolder;

        if (!fs::exists(archivePath)) {
            crow::json::wvalue error;
            error["error"] = "Archive not found";
            error["path"] = archivePath;
            return crow::response(404, error);
        }

        try {
            huffman::FolderCompressor compressor;
            
            auto start = std::chrono::high_resolution_clock::now();
            bool success = compressor.decompressArchive(archivePath, outputPath);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();

            if (!success) {
                crow::json::wvalue error;
                error["error"] = "Archive extraction failed";
                return crow::response(500, error);
            }

            crow::json::wvalue response;
            response["success"] = true;
            response["archive"] = archiveName;
            response["output_folder"] = outputFolder;
            response["time_ms"] = duration;

            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Exception occurred";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // API endpoint: GET /api/list - List files in uploads folder
    CROW_ROUTE(app, "/api/list")
    ([]() {
        crow::json::wvalue response;
        std::vector<crow::json::wvalue> files;

        try {
            for (const auto& entry : fs::directory_iterator("uploads")) {
                if (entry.is_regular_file()) {
                    crow::json::wvalue file;
                    file["name"] = entry.path().filename().string();
                    file["size"] = entry.file_size();
                    files.push_back(std::move(file));
                }
            }

            response["files"] = std::move(files);
            response["count"] = files.size();
            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Failed to list files";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // API endpoint: GET /api/info/<filename> - Get compressed file info
    CROW_ROUTE(app, "/api/info/<string>")
    ([](const std::string& filename) {
        std::string filePath = "compressed/" + filename;

        if (!fs::exists(filePath)) {
            crow::json::wvalue error;
            error["error"] = "File not found";
            error["path"] = filePath;
            return crow::response(404, error);
        }

        try {
            bool is_valid = huffman::isValidCompressedFile(filePath);
            
            crow::json::wvalue response;
            response["filename"] = filename;
            response["valid_huffman_file"] = is_valid;
            
            if (is_valid) {
                size_t size = huffman::getCompressedFileSize(filePath);
                response["compressed_size"] = size;
            }

            return crow::response(200, response);

        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["error"] = "Exception occurred";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    std::cout << "HuffmanCompressor API Server Starting..." << std::endl;
    std::cout << "Server will run on http://0.0.0.0:8080" << std::endl;
    std::cout << "\nAvailable endpoints:" << std::endl;
    std::cout << "  GET  / - API information" << std::endl;
    std::cout << "  POST /api/compress - Compress a file" << std::endl;
    std::cout << "  POST /api/decompress - Decompress a file" << std::endl;
    std::cout << "  POST /api/compress-folder - Compress a folder" << std::endl;
    std::cout << "  POST /api/decompress-folder - Decompress an archive" << std::endl;
    std::cout << "  GET  /api/list - List files in uploads" << std::endl;
    std::cout << "  GET  /api/info/<filename> - Get file info" << std::endl;
    std::cout << "  POST /api/tree-dot - Generate Huffman tree DOT file for a file in uploads" << std::endl;
    std::cout << "\nStarting server..." << std::endl;

    app.port(8081).multithreaded().run();


    
    return 0;
}
