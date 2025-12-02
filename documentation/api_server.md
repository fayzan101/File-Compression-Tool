# api_server.cpp Documentation (HTTP API)

## Overview
`api_server.cpp` exposes the compressor functionality as a RESTful HTTP API using the Crow C++ micro-framework. It supports:

- File compression and decompression.
- Folder compression and decompression.
- Listing upload files.
- Inspecting compressed file metadata.
- Generating Huffman tree DOT files for visualization.

The server is configured for Windows and listens on port 8081.

## Environment and Setup
- Defines Windows-specific macros for ASIO: `_WIN32_WINNT`, `ASIO_STANDALONE`, `ASIO_HAS_STD_*`.
- Includes core project headers (`HuffmanTree`, `HuffmanCompressor`, `FolderCompressor`, `CompressionSettings`).
- Uses `namespace fs = std::filesystem` for file system operations.

### Helper Functions
- `std::string readFile(const std::string& path)`: read an entire file as binary into a string.
- `void writeFile(const std::string& path, const std::string& content)`: write binary data to disk.

## Endpoints

### `POST /api/tree-dot`
- Request JSON: `{ "filename": "<uploaded-file-name>" }`.
- Reads `uploads/<filename>`, builds a frequency table, and constructs a `HuffmanTree`.
- Exports the tree to DOT via `toDot()`.
- Ensures a `dot/` directory exists.
- Strips the original extension using `std::filesystem::path::stem()` and writes `dot/<base>.dot`.
- Response JSON:
  - `success`: `true`.
  - `filename`: original file name.
  - `dot_file`: path to generated DOT file.

### `GET /`
- Returns a JSON overview with:
  - `message`: API description.
  - `version`: `huffman::getVersion()`.
  - `endpoints`: list of human-readable endpoint descriptions.

### `POST /api/compress`
- Request JSON: `{ "filename": "<name-in-uploads>", "level": <1-9> }`.
- Derives input path `uploads/<filename>`.
- Output archive name is `stem(filename) + ".zip"` under `compressed/`.
- Uses `huffman::make_settings_from_level(level)` and `huffman::compressFile`.
- Response JSON includes:
  - `success`, `filename`, `output`.
  - `original_size`: original size in KB (rounded to 2 decimals).
  - `compressed_size`: compressed size in KB.
  - `compression_ratio`: percentage (rounded to 2 decimals).
  - `time_ms`: compression time in ms (rounded).
  - `level`.

### `POST /api/decompress`
- Request JSON: `{ "filename": "<name-in-compressed>", "output": "optional-name" }`.
- If `output` is omitted, uses the stem of `filename` as the output basename.
- Input path: `compressed/<filename>`; output path: `decompressed/<outputName>`.
- Detects stored-only format by checking for `"STOR"` magic:
  - If stored, copies payload bytes after header to output.
  - Otherwise, calls `huffman::decompressFile`.
- Response JSON:
  - `success`, `filename`, `output`.
  - `size`: output size in KB (rounded to 2 decimals).
  - `time_ms`: decompression time in ms.

### `POST /api/compress-folder`
- Request JSON: `{ "folder": "<folder-in-uploads>", "archive": "<archive-base>", "level": <optional-level> }`.
- Builds archive path `compressed/<archive>.zip` and input folder `uploads/<folder>`.
- Uses `FolderCompressor::compressFolder` with target settings.
- After success, calls `getArchiveInfo` to summarize.
- Response JSON:
  - `success`, `folder`, `archive`, `file_count`.
  - `original_size`: total uncompressed size in KB.
  - `compressed_size`: total stored size in KB.
  - `compression_ratio`: percentage.
  - `time_ms`: time in ms (rounded).

### `POST /api/decompress-folder`
- Request JSON: `{ "archive": "<archive-base>", "output": "<output-folder>" }`.
- Operates on `compressed/<archive>.zip` and extracts into `decompressed/<output>`.
- Uses `FolderCompressor::decompressArchive`, then `getArchiveInfo` for aggregate sizes.
- Response JSON mirrors `/api/compress-folder`:
  - `success`, `archive`, `output_folder`, `file_count`.
  - `original_size`, `compressed_size` (both in KB), `compression_ratio`, `time_ms`.

### `GET /api/list`
- Lists all regular files in `uploads/`.
- Response JSON:
  - `files`: array of `{ name, size }` entries.
  - `count`: number of files.

### `GET /api/info/<filename>`
- Looks at `compressed/<filename>`.
- Uses `huffman::isValidCompressedFile` and `getCompressedFileSize`.
- Response JSON:
  - `filename`, `valid_huffman_file`.
  - `compressed_size` (in bytes) if valid.

## Logging and Startup
- Sets `app.loglevel(crow::LogLevel::Info)`.
- Logs startup banner and endpoint list to `std::cout`.
- Binds on port `8081` with `multithreaded().run()`.

This file is the primary integration point for external systems (frontends, tools) that want to call the compressor over HTTP instead of the CLI.
