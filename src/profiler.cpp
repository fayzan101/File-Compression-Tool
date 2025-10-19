#include "../include/Profiler.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <windows.h>
#include <psapi.h>

using namespace std;
using namespace util;

uint64_t util::getCurrentPeakRSS() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<uint64_t>(pmc.PeakWorkingSetSize);
    }
    return 0;
}

// Simple runner: accepts file paths in args and runs compressions using main CLI functions
// int main(int argc, char** argv) {
//     if (argc < 2) {
//         cout << "Usage: profiler <file1> [file2 ...]" << endl;
//         return 1;
//     }
//     vector<ProfileResult> results;
//     for (int i = 1; i < argc; ++i) {
//         string file = argv[i];
//         ProfileResult r;
//         r.file = file;
//         // Measure original size
//         ifstream in(file, ios::binary | ios::ate);
//         if (!in) {
//             cerr << "Cannot open " << file << endl;
//             continue;
//         }
//         r.original_size = in.tellg();
//         in.close();
//
//         // Run single-threaded Huffman via CLI wrapper
//         string out_huf = file + ".huf.prof";
//         auto t0 = chrono::high_resolution_clock::now();
//         // Call library API
//         {
//             auto start_rss = util::getCurrentPeakRSS();
//             // invoke compressFile function from huffman namespace using system call for simplicity
//             string cmd = "\"huffman_compressor.exe\" compress \"" + file + "\" \"" + out_huf + "\"";
//             system(cmd.c_str());
//             r.peak_rss = max<uint64_t>(r.peak_rss, util::getCurrentPeakRSS());
//         }
//         auto t1 = chrono::high_resolution_clock::now();
//         r.time_ms = chrono::duration<double, milli>(t1 - t0).count();
//         // compressed size
//         ifstream ou(out_huf, ios::binary | ios::ate);
//         if (ou) r.compressed_size = ou.tellg();
//         ou.close();
//         r.mode = "huffman";
//         results.push_back(r);

//         // Cleanup
//         remove(out_huf.c_str());
//     }

//     // Print CSV
//     cout << "mode,file,original,compressed,time_ms,peak_rss\n";
//     for (const auto& res : results) {
//         cout << res.mode << "," << res.file << "," << res.original_size << "," << res.compressed_size << "," << res.time_ms << "," << res.peak_rss << "\n";
//     }
//     return 0;
// }
