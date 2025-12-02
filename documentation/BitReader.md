# BitReader.cpp Documentation

## Overview
`BitReader` provides bit-level reading over an in-memory byte buffer. It is used by the Huffman decompression logic to interpret variable-length Huffman codes from a compressed bitstream.

## Core Concepts
- **Buffered bit access**: Wraps a `std::vector<uint8_t>` and exposes operations to read individual bits or a fixed number of bits.
- **Bit position tracking**: Maintains `byte_pos_` and `bit_pos_` to know which bit in the buffer is next.
- **Sequential consumption**: Reading is forward-only; each call advances the internal cursor.

## Key Functions
- `BitReader(const std::vector<uint8_t>& buffer)`: stores a reference copy of the input buffer and initializes internal cursors.
- `bool readBit()`: 
  - Returns the next bit as a `bool` (`true` for 1, `false` for 0).
  - Safely handles end-of-buffer by returning `false` if there are no more bytes.
  - Shifts through bits from MSB to LSB within each byte, then advances to the next byte.
- `uint64_t readBits(unsigned count)`: 
  - Repeatedly calls `readBit()` `count` times and left-shifts into a 64-bit accumulator.
  - Used to reconstruct fixed-width fields from the compressed bitstream.
- `bool hasMoreBits() const`: 
  - Returns whether there are unread bytes remaining (at least one byte not fully consumed).

## Usage in the Project
- Used extensively in `Decompressor.cpp` to walk through Huffman-coded data (both single-stream and per-chunk), constructing codewords one bit at a time until a valid code is recognized.
