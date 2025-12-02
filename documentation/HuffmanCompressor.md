# HuffmanCompressor.cpp Documentation (Library Facade)

## Overview
`HuffmanCompressor.cpp` provides high-level, library-friendly functions in the `huffman` namespace that wrap the lower-level `Compressor` and `Decompressor` classes. It exposes:

- Stream-based `compress` / `decompress` functions.
- Buffer-based convenience wrappers (`compressBuffer`, `decompressBuffer`).
- File-based helpers with timing and ratio metrics (`compressFile`, `decompressFile`).
- Validation and introspection utilities (`isValidCompressedFile`, `getCompressedFileSize`, `getVersion`).

All results are returned via a `CompressionResult` struct that carries sizes, timings, and error status.

## Stream-Based API
- `CompressionResult compress(std::istream& in, std::ostream& out, const CompressionSettings& settings)`:
  - Reads all bytes from `in` into memory.
  - Computes `original_size` and `original_checksum` (CRC32) for diagnostics.
  - For empty input, writes a minimal `HUF1` header (empty legacy file) and returns success.
  - For non-empty input:
    - Writes input data to a temporary file `temp_lib_input.txt`.
    - Invokes `Compressor::compress` to produce `temp_lib_compressed.huf` using given `settings`.
    - Streams compressed data into `out` and populates `compressed_size`.
    - Deletes temporary files.
  - Measures wall-clock compression time and computes `compression_ratio` as a percentage.

- `CompressionResult decompress(std::istream& in, std::ostream& out)`:
  - Reads compressed bytes from `in` into memory.
  - Writes them to `temp_lib_compressed.huf` and calls `Decompressor::decompress` to produce `temp_lib_output.txt`.
  - Streams the decompressed result into `out`, sets `original_size`, and deletes temps.
  - Measures decompression time.

## Buffer-Based API
- `std::vector<uint8_t> compressBuffer(const std::vector<uint8_t>& in, const CompressionSettings& settings)`:
  - Wraps the input in a `std::stringstream`, calls the stream-based `compress`, and then returns the compressed bytes.
  - Returns an empty vector if compression fails.
- `std::vector<uint8_t> decompressBuffer(const std::vector<uint8_t>& in)`:
  - Symmetric buffer-to-buffer decompression wrapper.

These buffer APIs are heavily used by `FolderCompressor` when compressing individual files inside an archive.

## File-Based Helpers
- `CompressionResult compressFile(const std::string& inPath, const std::string& outPath, const CompressionSettings& settings)`:
  - Measures source file size (via `std::ifstream` with `std::ios::ate`).
  - Constructs a `Compressor` and calls its `compress` method on the paths.
  - After success, computes compressed file size similarly and fills `CompressionResult`.
- `CompressionResult decompressFile(const std::string& inPath, const std::string& outPath)`:
  - Measures compressed size, then uses a `Decompressor` to expand to `outPath`.
  - Measures decompressed size and reports it in `CompressionResult`.

## Validation and Metadata
- `bool isValidCompressedFile(const std::string& path)`:
  - Opens the file and reads up to 8 magic bytes.
  - Accepts any file starting with:
    - `"HUF1"` (legacy empty/single-chunk).
    - `"HUF2"` (legacy Huffman-only with header).
    - `"HUF_LZ77"` (hybrid LZ77 + Huffman).
    - `"HUF_PAR"` (parallel container).
- `size_t getCompressedFileSize(const std::string& path)`:
  - Returns the file size via `std::ios::ate`.
- `std::string getVersion()`:
  - Currently hard-coded to `"1.0.0"`.

## Design Notes
- This file acts as an abstraction layer so callers (CLI, API server, other applications) can use simple function calls without caring about individual formats or temporary-file handling.
- Use of temporary files keeps the compressor/decompressor code simple and file-centric, at the cost of some disk I/O; the buffer and stream wrappers compensate when in-memory usage is needed (e.g., archives, HTTP).
