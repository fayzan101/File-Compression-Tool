# Source File Documentation Index

This folder contains per-implementation documentation for the core `.cpp` files under `src/`.

- `BitReader.md` – Bit-level buffered reader used in decompression.
- `BitWriter.md` – Bit-level buffered writer used in compression.
- `Checksum.md` – CRC32 checksum implementation for integrity checking.
- `Compressor.md` – Core compressor implementation, including hybrid LZ77 + Huffman and parallel chunked compression.
- `Decompressor.md` – Core decompressor that understands all supported formats.
- `FolderCompressor.md` – Folder-level archive format and operations.
- `HuffmanCompressor.md` – Library facade/wrapper API for compression and decompression.
- `HuffmanTree.md` – Huffman tree construction, canonical code generation, and DOT export.
- `LZ77.md` – LZ77 tokenization and detokenization used in the hybrid pipeline.
- `main_cli.md` – Interactive command-line interface implementation.
- `profiler.md` – Windows-specific peak RSS memory profiling helper.
- `api_server.md` – HTTP REST API server exposing compressor functionality.

Each document explains the core concepts, algorithms, and interactions specific to its corresponding `.cpp` file.
