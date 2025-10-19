#pragma once

#include <string>
#include <cstdint>

namespace util {

struct ProfileResult {
    std::string mode;
    std::string file;
    uint64_t original_size = 0;
    uint64_t compressed_size = 0;
    double time_ms = 0.0;
    uint64_t peak_rss = 0; // bytes, best-effort
};

// Measure current process peak RSS (best-effort)
uint64_t getCurrentPeakRSS();

} // namespace util
