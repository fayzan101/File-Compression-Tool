# Compressor.cpp and LZ77 Hybrid Compression Documentation

## Overview
`Compressor.cpp` (together with the first part of `LZ77.cpp`) implements the core file compression logic for the project. It supports:

- Classic Huffman-only compression (legacy formats `HUF1` / `HUF2`).
- A hybrid LZ77 + Huffman pipeline for better compression on repetitive data (`HUF_LZ77`).
- Parallel chunked Huffman compression (`HUF_PAR`) for large files.

All entry points are methods on the `Compressor` class.

## Parallel Compression (`compressParallel`)
Defined at the top of `LZ77.cpp` as a `Compressor` method.

### High-Level Flow
1. **Read entire input file** into a `std::vector<unsigned char>`.
2. **Fallback for empty file**: delegates to `compressInternal` to preserve legacy empty handling.
3. **Split into chunks** using `splitChunks(data, chunkSize)`:
   - Splits the buffer into contiguous slices of `chunkSize` bytes.
4. **Compress each chunk in parallel** using `std::async`:
   - For each chunk `i`:
     - Build a local symbol frequency table.
     - Build a `HuffmanTree`, derive canonical codes and code lengths.
     - Use `BitWriter` to encode all bytes of the chunk.
     - Compute CRC32 over the compressed bit buffer.
     - Build a self-contained chunk blob:
       - Magic `"HUF2"`.
       - Original chunk size (`uint64_t`, little-endian).
       - 256 code lengths (one byte per symbol).
       - CRC32 (little-endian bytes).
       - Raw compressed bitstream.
     - Store this chunk blob into `compressedChunks[i]` and record `chunkSizes[i]`.
   - Uses mutex-protected writes into shared vectors and an atomic counter for progress display.
5. **Write container file**:
   - File header: magic `"HUF_PAR"` (7 bytes) + number of chunks (`uint32_t`).
   - Then an array of per-chunk sizes (`uint32_t` each).
   - Then concatenates all chunk blobs back-to-back.

### Core Concepts
- **Embarrassingly parallel compression**: Each chunk is compressed independently with its own Huffman model, trading some compression efficiency for speed.
- **Self-describing chunks**: Each chunk carries its codebook and CRC so it can be validated and decompressed independently.
- **Progress reporting**: Optional textual progress bar when `settings.progress` is enabled.

## Hybrid LZ77 + Huffman (`compressInternal`)
Defined in `Compressor.cpp` as the primary non-parallel compressor.

### High-Level Flow
1. **Open input file** and read entire contents into `input_data`.
2. **Handle empty input**:
   - Writes magic `"HUF1"`, table size `0`, and returns (legacy empty format).
3. **LZ77 stage**:
   - Calls `LZ77::compress(input_data)` to produce a sequence of `(offset, length, next)` tokens.
   - Serializes tokens to bytes with `LZ77::tokensToBytes`, giving `lz_bytes`.
4. **Frequency counting** over `lz_bytes` to build a symbol histogram.
5. **Huffman model build**:
   - Builds `HuffmanTree` from frequencies.
   - Uses `getCanonicalCodes()` to compute canonical codewords per symbol.
6. **Write hybrid header**:
   - Magic `"HUF_LZ77"` (8 bytes).
   - 256 code length bytes (one per possible symbol); 0 means unused.
7. **Encode data**:
   - Use `BitWriter` to emit bits for each `lz_bytes` symbol using the canonical codes.
   - Flush to obtain `buf` (compressed bitstream).
8. **CRC32 and payload**:
   - Compute CRC32 over `buf`.
   - Write CRC32 (4 bytes) to file.
   - Append raw contents of `buf`.

### Error Handling
- Wraps logic in `try/catch` for `HuffmanError` and `std::exception`.
- Converts several error codes to user-friendly suggestions (file not found, I/O errors, invalid headers, etc.).

## Public API Methods
- `bool Compressor::compress(const std::string& inPath, const std::string& outPath)`:
  - Convenience wrapper using default compression level (5).
- `bool Compressor::compress(const std::string& inPath, const std::string& outPath, const CompressionSettings& settings)`:
  - Calls `compressInternal` with specified settings.
- `bool Compressor::compressInternal(...)`:
  - Full hybrid LZ77+Huffman pipeline as described above, including header and CRC.
- `bool Compressor::compressParallel(...)`:
  - Parallel chunked compressor building `HUF_PAR` container files.

## Interaction with Other Components
- **`LZ77`**: Provides pre-compression via dictionary-based match copying.
- **`HuffmanTree`**: Builds canonical Huffman code lengths and bit patterns.
- **`BitWriter`**: Packs variable-length codes into bytes.
- **`Checksum` (CRC32)**: Adds integrity checks to compressed streams.
- **`ErrorHandler` / `HuffmanError`**: Reports structured errors with codes and suggestions.
