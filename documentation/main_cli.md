# main_cli.cpp Documentation (Interactive CLI)

## Overview
`main_cli.cpp` implements a text-based interactive menu for the HuffmanCompressor application. It allows users to:

1. Compress individual files.
2. Perform hybrid LZ77 + Huffman compression.
3. Decompress files (including stored-only files).
4. Compress folders into multi-file archives.
5. Inspect archive contents.
6. Inspect compressed files (size and validity).
7. Export Huffman trees as DOT graphs.
8. Show a detailed help/usage screen.

It uses the `HuffmanCLI` helper class internally to parse options and coordinate operations with the underlying library.

## Configuration and Options
- `struct HuffmanCLI::Options`:
  - `command`: not heavily used in the interactive menu, but part of the design.
  - `input_file`, `output_file`: paths for file operations.
  - `level`: compression level 1–9 (speed vs. ratio tradeoff).
  - `verbose`, `progress`, `verify`: flags controlling logging and verification.
- `static void loadConfig(Options& opts)`:
  - Reads `config.ini`, section `[defaults]`, for keys:
    - `level`, `verbose`, `progress`, `verify`.
  - Sets default CLI behavior based on simple key-value lines.

## Help / Usage
- `static void printUsage()`:
  - Prints a rich, user-friendly help covering:
    - File and folder operations, archive listing, info, and tree visualization.
    - Folder structure conventions (`uploads/`, `compressed/`, `decompressed/`).
    - Format notes (Huffman vs stored `STOR` vs folder archives).
    - Tips and best practices for compression levels and data types.

## Argument Parsing (Non-interactive mode)
Although the menu is interactive, `parseArguments` exists to support a more traditional CLI:
- Interprets flags:
  - `--help / -h`, `--verbose / -v`, `--progress / -p`, `--verify`.
  - `--level / -l <1-9>`.
- Collects positional `input_file` and `output_file`.

## File Compression (`compressFile`)
- Accepts `Options` and flags for parallel vs hybrid modes.
- Validates input/output paths and compression level.
- Constructs `CompressionSettings` from level and verbosity.
- For **hybrid mode**:
  - Directly calls `Compressor::compressInternal` (LZ77 + Huffman), then prints sizes and ratio.
- For **parallel mode**:
  - Calls `Compressor::compressParallel` with a 1MB chunk size and prints metrics.
- For **standard mode**:
  - Uses `huffman::compressFile` (library wrapper).
  - If compressed size is not at least ~5% smaller than original, replaces output with a stored `STOR` format file:
    - Header `"STOR" + [uint64 length] + raw data`.

## File Decompression (`decompressFile`)
- Detects stored-only files by checking for `"STOR"` magic.
  - If found, simply copies the payload after header to the output.
- Otherwise, calls `huffman::decompressFile` and reports compressed/decompressed sizes and time.

## Info / Validation (`showFileInfo`)
- Uses `huffman::isValidCompressedFile` and `getCompressedFileSize` to:
  - Report whether a file looks like a valid Huffman-compressed file.
  - Provide its size if valid.

## Folder Operations
- **Option 4: Compress folder**:
  - Lists available subdirectories in `uploads/`.
  - Asks for folder name and archive name.
  - Forces `.zip` extension and writes to `compressed/`.
  - Uses `FolderCompressor::compressFolder` with progress callback.
  - After success, prints aggregated archive statistics.
- **Option 5: List archive files**:
  - Uses `FolderCompressor::isValidArchive` and `getArchiveInfo`.
  - Prints file list, per-file sizes, and whether each entry is compressed vs stored.

## Huffman Tree Export (Option 7)
- Lists available files in `uploads/`.
- Asks the user to choose an input file and a DOT output name.
- Builds symbol frequency table, `HuffmanTree`, and then writes `tree.toDot()` into `dot/<name>.dot`.

## Main Loop
- Presents a numeric menu repeatedly until the user chooses `0`, `exit`, or `quit`.
- Handles all exceptions from `HuffmanError` and generic `std::exception`, printing suggestions for:
  - Missing files, I/O errors, invalid headers, decompression failures, invalid input, memory issues.

This CLI is the primary human-facing frontend when running the compressor outside of the HTTP API server.
