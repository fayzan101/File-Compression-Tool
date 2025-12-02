# Checksum.cpp Documentation

## Overview
`Checksum.cpp` implements a CRC32 checksum facility in the `huffman` namespace. It is used to verify integrity of compressed data blocks and detect corruption.

## Core Concepts
- **CRC32 polynomial**: Uses a standard 32-bit CRC with a precomputed lookup table (`crc32_table[256]`).
- **Streaming checksum**: Processes data byte-by-byte, updating a 32-bit state.
- **Endianness and representation**: Stores the final CRC as a 32-bit unsigned integer, with helpers to convert to/from hexadecimal string form.

## Key Functions
- `uint32_t CRC32::calculate(const std::vector<uint8_t>& data)`: Convenience overload to compute CRC of a byte vector.
- `uint32_t CRC32::calculate(const std::string& data)`: Computes CRC over a string by reinterpreting its bytes.
- `uint32_t CRC32::calculate(const uint8_t* data, size_t length)`: Core implementation:
  - Initializes `crc` to `0xFFFFFFFF`.
  - For each byte, computes `table_index = (crc ^ byte) & 0xFF` and updates `crc = (crc >> 8) ^ crc32_table[table_index]`.
  - Finalizes with `crc ^ 0xFFFFFFFF`.
- `std::string CRC32::toHex(uint32_t crc)`: Formats the CRC as an 8-character uppercase hexadecimal string with leading zeros.
- `uint32_t CRC32::fromHex(const std::string& hex)`: Parses a hex string back into a 32-bit CRC using `std::stoul`.

## Usage in the Project
- Used in `Compressor.cpp` and `Decompressor.cpp` to protect the compressed bitstream (CRC over the Huffman-coded bytes).
- Used in `LZ77.cpp` chunked parallel compression to attach a CRC per chunk.
- Used in `FolderCompressor.cpp` to store per-file checksums inside folder archives.
- Enables robust corruption detection at both the file and archive-chunk level.
