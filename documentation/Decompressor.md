# Decompressor.cpp Documentation

## Overview
`Decompressor` reverses all compression formats produced by the `Compressor`:

- Parallel chunked Huffman (`HUF_PAR` container of `HUF2` chunks).
- Hybrid LZ77 + Huffman (`HUF_LZ77`).
- Legacy Huffman-only (`HUF1` / `HUF2`).
- It also interoperates with the CLI and library glue in `HuffmanCompressor.cpp`.

## Core Concepts
- **Magic-based format dispatch**: Reads up to 8 bytes of magic and chooses a decoding path.
- **Canonical code reconstruction**: Rebuilds canonical Huffman codes from stored code lengths.
- **CRC32 verification**: Validates compressed data against stored CRC before decoding.
- **Optional LZ77 post-processing**: For hybrid streams, decodes LZ77 tokens after Huffman.

## Parallel Container Handling (`HUF_PAR`)
1. Read 7-byte magic `"HUF_PAR"` and a `uint32_t` chunk count.
2. Read an array of `chunkSizes` (`uint32_t` per chunk).
3. For each chunk:
   - Read its raw blob of length `sz`.
   - Ensure it begins with `"HUF2"` (per-chunk magic).
   - Optionally parse original uncompressed size (`uint64_t`) if present in the chunk.
   - Read 256 code lengths and build `code_lens`.
   - Read CRC32, then remaining bytes (`crc_buf`) as compressed bitstream.
   - Verify CRC32 matches `crc_buf`.
   - Reconstruct canonical codes and a reverse map `rev_codes` from bitstrings to symbols.
   - Use `BitReader` to stream bits, accumulate them into a temporary string, and emit symbols when a codeword is recognized, stopping when `orig_uncompressed` bytes have been produced (if this field exists).
   - Append decoded bytes to `final_out`.
4. After all chunks, write `final_out` to the output file.

## Non-Parallel Formats
After ruling out `HUF_PAR`, `Decompressor` interprets other magic strings:

- `HUF_LZ77...` -> hybrid path (`is_hybrid = true`).
- `HUF2` / `HUF1` -> legacy Huffman-only.
- Otherwise -> `INVALID_MAGIC` error.

### Shared Steps
1. **Code length table**: Read 256 bytes, building `code_lens`.
2. **Empty file shortcut**: If no code lengths are non-zero, write an empty file and return.
3. **CRC32**: Read stored CRC, then all remaining bytes into `crc_buf` and verify CRC.
4. **Canonical reconstruction**:
   - Sort `(symbol, length)` pairs by length then symbol.
   - Assign canonical integer codes and convert to bitstrings.
   - Build `rev_codes` mapping bitstrings to bytes.
5. **Huffman decode**:
   - Stream bits from `crc_buf` via `BitReader`.
   - Grow a current code string until it matches `rev_codes`, then emit symbol and reset.

### Hybrid LZ77 + Huffman (HUF_LZ77)
- After Huffman decode, `decoded` represents serialized LZ77 tokens:
  - Convert bytes to `LZ77::Token` vector using `LZ77::bytesToTokens`.
  - Run `LZ77::decompress(tokens)` to reconstruct the original byte stream into `final_output`.

### Legacy Huffman (HUF1/HUF2)
- Skips the LZ77 phase: `final_output` is just the Huffman-decoded data.

## Error Handling
- Throws `HuffmanError` with detailed codes for:
  - Missing or truncated magic / headers.
  - Code-length table read errors.
  - CRC mismatches.
  - Missing compressed data.
- Catches and reports both `HuffmanError` and generic `std::exception`, printing suggestions for common scenarios (I/O permissions, corruption, memory issues).

## Interaction with Other Components
- **`BitReader`**: For bitwise traversal of compressed Huffman streams.
- **`Checksum` (CRC32)**: Validates compressed blocks.
- **`LZ77`**: Reconstructs data in hybrid mode.
- **`HuffmanTree`**: Provides helper `bitstring()` (local static function) to format canonical codes.
- **`HuffmanCompressor`**: Library-level wrappers in `HuffmanCompressor.cpp` delegate to `Decompressor::decompress`.
