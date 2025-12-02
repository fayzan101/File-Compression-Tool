# BitWriter.cpp Documentation

## Overview
`BitWriter` provides bit-level writing into an internal byte buffer. It is used whenever the compressor must emit variable-length Huffman codes to a contiguous bitstream.

## Core Concepts
- **Bit packing**: Individual bits are accumulated into `current_byte_` and flushed to `buffer_` once 8 bits are collected.
- **Big-endian bit order within a byte**: Bits are shifted left and OR-ed so the first written bit becomes the most significant bit.
- **Buffered output**: You first build an in-memory compressed bit buffer, then optionally flush it to an `std::ostream`.

## Key Functions
- `BitWriter()`: Initializes an empty buffer, zeroed `current_byte_`, and `bit_pos_ = 0`.
- `void writeBit(bool bit)`: 
  - Shifts `current_byte_` and sets its LSB based on `bit`.
  - Increments `bit_pos_` and, once 8 bits are accumulated, pushes the full byte to `buffer_` and resets state.
- `void writeBits(uint64_t value, unsigned count)`: 
  - Writes the high `count` bits of `value`, from most significant to least, by repeatedly calling `writeBit`.
  - This is used to output canonical Huffman codes represented as integers.
- `void flush()`: 
  - If the current byte is only partially filled, left-pads the remaining bits with zeros and appends it to `buffer_`.
  - Ensures the buffer is byte-aligned before consumers read from it.
- `const std::vector<uint8_t>& getBuffer() const`: Exposes the internal compressed byte buffer.
- `void writeToStream(std::ostream& os)`: Flushes pending bits and writes all buffered bytes to an output stream.

## Usage in the Project
- Used in `Compressor.cpp` and `LZ77.cpp` to encode Huffman codewords into a compact bitstream.
- The resulting `buffer_` is then written into the compressed file format after headers (magic, code lengths, CRC, etc.).
