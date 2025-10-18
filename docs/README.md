# HuffmanCompressor

A high-performance C++ Huffman compression library with support for multiple compression levels and a clean API.

## Features

- ✅ **High Performance**: Optimized Huffman coding implementation
- ✅ **Multiple Compression Levels**: 9 levels from fast (-1) to best (-9)
- ✅ **Clean API**: Both file-based and stream-based interfaces
- ✅ **Error Handling**: Comprehensive error reporting
- ✅ **Cross-Platform**: Works on Windows, Linux, and macOS
- ✅ **Well Tested**: Comprehensive unit and integration tests

## Quick Start

### Basic Usage

```cpp
#include "include/HuffmanCompressor.h"

// File compression
auto result = huffman::compressFile("input.txt", "output.huf");
if (result.success) {
    std::cout << "Compressed: " << result.original_size 
              << " -> " << result.compressed_size << " bytes" << std::endl;
}

// File decompression
auto decomp_result = huffman::decompressFile("output.huf", "restored.txt");
```

### Buffer Compression

```cpp
std::vector<uint8_t> data = { /* your data */ };
auto compressed = huffman::compressBuffer(data);
auto decompressed = huffman::decompressBuffer(compressed);
```

### Stream Compression

```cpp
std::stringstream input, output;
// ... fill input stream ...
auto result = huffman::compress(input, output);
```

## Compression Levels

| Level | Mode | Description | Use Case |
|-------|------|-------------|----------|
| 1-3 | Fast | Quick compression, lower ratio | Real-time applications |
| 4-6 | Default | Balanced speed/ratio | General purpose |
| 7-9 | Best | Maximum compression | Archival storage |

### Example with Different Levels

```cpp
// Fast compression
auto fast_settings = huffman::make_settings_from_level(1);
auto fast_result = huffman::compressFile("input.txt", "fast.huf", fast_settings);

// Best compression
auto best_settings = huffman::make_settings_from_level(9);
auto best_result = huffman::compressFile("input.txt", "best.huf", best_settings);
```

## API Reference

### Core Functions

#### `compressFile(inPath, outPath, settings)`
Compress a file to a Huffman-compressed file.

**Parameters:**
- `inPath`: Input file path
- `outPath`: Output compressed file path  
- `settings`: Compression settings (optional, defaults to level 5)

**Returns:** `CompressionResult` with success status, sizes, and timing

#### `decompressFile(inPath, outPath)`
Decompress a Huffman-compressed file.

**Parameters:**
- `inPath`: Compressed file path
- `outPath`: Output decompressed file path

**Returns:** `CompressionResult` with success status and timing

#### `compressBuffer(data, settings)`
Compress data from a buffer.

**Parameters:**
- `data`: Input data as `std::vector<uint8_t>`
- `settings`: Compression settings (optional)

**Returns:** Compressed data as `std::vector<uint8_t>`

#### `decompressBuffer(data)`
Decompress data from a buffer.

**Parameters:**
- `data`: Compressed data as `std::vector<uint8_t>`

**Returns:** Decompressed data as `std::vector<uint8_t>`

### Utility Functions

#### `isValidCompressedFile(path)`
Check if a file is a valid Huffman-compressed file.

#### `getCompressedFileSize(path)`
Get the size of a compressed file.

#### `getVersion()`
Get the library version string.

## Error Handling

The library provides comprehensive error handling through the `CompressionResult` structure:

```cpp
auto result = huffman::compressFile("input.txt", "output.huf");
if (!result.success) {
    std::cerr << "Error: " << result.error << std::endl;
}
```

### Error Codes

- `FILE_NOT_FOUND`: Input file doesn't exist
- `FILE_READ_ERROR`: Cannot read input file
- `FILE_WRITE_ERROR`: Cannot write output file
- `INVALID_MAGIC`: Invalid compressed file format
- `CORRUPTED_HEADER`: Corrupted file header
- `DECOMPRESSION_FAILED`: Decompression process failed
- `COMPRESSION_FAILED`: Compression process failed

## Building

### Requirements
- C++17 or later
- CMake 3.10+ (optional)

### Quick Build
```bash
# Compile all source files
g++ src/*.cpp -I include -std=c++17 -O2 -o huffman_compressor

# Run tests
g++ tests/*.cpp src/*.cpp -I include -std=c++17 -O2 -o test_runner
./test_runner
```

### CMake Build (Optional)
```bash
mkdir build && cd build
cmake ..
make
```

## Testing

The project includes comprehensive tests:

```bash
# Unit tests
./bit_io_test
./huffman_tree_test

# Integration tests  
./integration_test

# Performance tests
./performance_test
```

## Examples

See the `examples/` directory for complete usage examples:

- `library_example.cpp`: Demonstrates all API features
- `compression_levels.cpp`: Shows different compression levels
- `stream_processing.cpp`: Stream-based compression

## File Format

The compressed file format is:

```
Header:
- Magic: "HUF1" (4 bytes)
- Table Size: uint16_t (2 bytes)
- Frequency Table: (symbol, frequency) pairs
- Compressed Data: Bit stream
```

## Performance

Typical compression ratios:
- Text files: 40-60% of original size
- Repetitive data: 20-40% of original size  
- Random data: 95-105% of original size (overhead)

## License

This project is open source. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## Changelog

### Version 1.0.0
- Initial release
- Basic Huffman compression/decompression
- Multiple compression levels
- Comprehensive error handling
- Full test suite