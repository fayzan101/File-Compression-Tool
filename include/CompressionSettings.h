#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

using namespace std;

namespace huffman {

struct CompressionSettings {
    unsigned level = 5; // 1..9
    enum Mode { FAST = 0, DEFAULT = 1, BEST = 2 } mode = DEFAULT;
    size_t block_size = 0; // 0 = whole file
    bool canonicalize = true;
    unsigned extra_passes = 0;
    bool sampling = false;
    bool prefer_speed = false;
    
    // Additional settings for fine-tuning
    bool verbose = false;
    bool preserve_timestamps = false;
    std::string comment = "";
};

inline CompressionSettings make_settings_from_level(unsigned level) {
    CompressionSettings s;
    if (level < 1) level = 5;
    if (level <= 3) {
        s.level = level;
        s.mode = CompressionSettings::FAST;
        s.block_size = 64 * 1024; // 64 KiB
        s.canonicalize = false;
        s.extra_passes = 0;
        s.sampling = true;
        s.prefer_speed = true;
    } else if (level <= 6) {
        s.level = level;
        s.mode = CompressionSettings::DEFAULT;
        s.block_size = 0; // whole file when possible
        s.canonicalize = true;
        s.extra_passes = 0;
        s.sampling = false;
        s.prefer_speed = false;
    } else {
        s.level = level;
        s.mode = CompressionSettings::BEST;
        s.block_size = 0;
        s.canonicalize = true;
        s.extra_passes = 1;
        s.sampling = false;
        s.prefer_speed = false;
    }
    return s;
}

} // namespace huffman
