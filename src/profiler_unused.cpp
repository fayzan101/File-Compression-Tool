// This file was renamed to prevent accidental compilation and linker errors.
// Original filename: profiler.cpp
// To restore, rename back to profiler.cpp.

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
//     ...existing code...
// }
