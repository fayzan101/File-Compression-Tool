#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cctype>
#include "../include/HuffmanApi.h"

// Force Windows 10 (or later) target for cpp-httplib when compiling on Windows.
// Some toolchains or headers may predefine _WIN32_WINNT to an older value
// (e.g., Windows 8), which triggers a compile-time #error inside httplib.h.
// Undefine and set it explicitly to 0x0A00 (Windows 10) so the header will
// accept the platform.
#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// NOTE: This file depends on the single-header library cpp-httplib (https://github.com/yhirose/cpp-httplib)
// Place the header at: include/httplib.h
#include "httplib.h"

namespace fs = std::filesystem;

static std::string gen_id() {
    // timestamp + random hex
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::mt19937_64 rng(static_cast<unsigned long long>(ms));
    std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)-1);
    std::ostringstream oss;
    oss << std::hex << ms << "_" << dist(rng);
    return oss.str();
}

static std::string safe_filename(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (std::isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-') out += c;
        else out += '_';
    }
    return out;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc >= 2) port = std::stoi(argv[1]);

    fs::path uploads_dir = "uploads";
    fs::path results_dir = "results";
    fs::create_directories(uploads_dir);
    fs::create_directories(results_dir);

    httplib::Server svr;

    svr.Post("/compress", [&](const httplib::Request& req, httplib::Response& res) {
        // Expect multipart form field "file" and optional form field "level"
        if (!req.form.has_file("file")) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"No file field 'file' in multipart form\"}", "application/json");
            return;
        }

        int level = 5;
        if (req.form.has_field("level")) {
            try { level = std::stoi(req.form.get_field("level")); } catch(...) { level = 5; }
        }

        auto file = req.form.get_file("file");
        std::string id = gen_id();
        std::string orig = safe_filename(file.filename.empty() ? ("upload_" + id) : file.filename);
        fs::path uploaded = uploads_dir / (id + "_" + orig);
        std::ofstream ofs(uploaded, std::ios::binary);
        ofs.write(file.content.data(), static_cast<std::streamsize>(file.content.size()));
        ofs.close();

        // Call compression
        std::string outname = id + "_" + orig + ".huf";
        fs::path outpath = results_dir / outname;
        bool ok = huffman_api::compressFile(uploaded.string(), outpath.string(), level, false);
        if (!ok) {
            res.status = 500;
            res.set_content("{\"success\":false,\"error\":\"Compression failed\"}", "application/json");
            return;
        }

        // Return JSON with download URL
        std::ostringstream js;
        js << "{\"success\":true,\"url\":\"/download/" << outname << "\",\"path\":\"" << outpath.string() << "\"}";
        res.set_content(js.str(), "application/json");
    });

    svr.Post("/decompress", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.form.has_file("file")) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"No file field 'file' in multipart form\"}", "application/json");
            return;
        }

        auto file = req.form.get_file("file");
        std::string id = gen_id();
        std::string orig = safe_filename(file.filename.empty() ? ("upload_" + id) : file.filename);
        fs::path uploaded = uploads_dir / (id + "_" + orig);
        std::ofstream ofs(uploaded, std::ios::binary);
        ofs.write(file.content.data(), static_cast<std::streamsize>(file.content.size()));
        ofs.close();

        // Call decompression
        std::string outname = id + "_" + orig + ".out";
        fs::path outpath = results_dir / outname;
        bool ok = huffman_api::decompressFile(uploaded.string(), outpath.string(), false);
        if (!ok) {
            res.status = 500;
            res.set_content("{\"success\":false,\"error\":\"Decompression failed\"}", "application/json");
            return;
        }

        std::ostringstream js;
        js << "{\"success\":true,\"url\":\"/download/" << outname << "\",\"path\":\"" << outpath.string() << "\"}";
        res.set_content(js.str(), "application/json");
    });

    // Serve results (download)
    svr.Get(R"(/download/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        auto matches = req.matches;
        if (matches.size() < 2) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"Missing filename\"}", "application/json");
            return;
        }
        std::string fname = matches[1];
        // prevent path traversal
        if (fname.find("..") != std::string::npos) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"Invalid filename\"}", "application/json");
            return;
        }
        fs::path target = results_dir / fname;
        if (!fs::exists(target)) {
            res.status = 404;
            res.set_content("{\"success\":false,\"error\":\"File not found\"}", "application/json");
            return;
        }
        res.set_content_provider(
            fs::file_size(target),
            "application/octet-stream",
            [target](size_t offset, size_t length, httplib::DataSink& sink) {
                std::ifstream ifs(target, std::ios::binary);
                ifs.seekg(offset, std::ios::beg);
                const size_t bufsize = 64 * 1024;
                std::vector<char> buf(std::min(bufsize, length));
                size_t remaining = length;
                while (remaining > 0 && ifs) {
                    size_t toread = std::min(buf.size(), remaining);
                    ifs.read(buf.data(), toread);
                    std::streamsize r = ifs.gcount();
                    if (r <= 0) break;
                    sink.write(buf.data(), static_cast<size_t>(r));
                    remaining -= static_cast<size_t>(r);
                }
                return true;
            });
        res.set_header("Content-Disposition", std::string("attachment; filename=\"") + fname + "\"");
    });

    std::cout << "Starting REST server on port " << port << "\n";
    svr.listen("0.0.0.0", port);

    return 0;
}
