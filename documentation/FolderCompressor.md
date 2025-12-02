# FolderCompressor.cpp Documentation

## Overview
`FolderCompressor` encapsulates creation and extraction of multi-file archives (folder-level compression). It supports:

- Recursively packaging a directory into a single archive file.
- Storing per-file metadata (relative path, timestamps, sizes, checksum, compression flag).
- Selective compression vs. store-only mode based on compression effectiveness.
- Progress callbacks to integrate with UIs/CLI.

Archives use a custom binary format beginning with a magic number `ARCHIVE_MAGIC` (`"HFAR"` in the headers).

## Key Data Structures
- **`ArchiveHeader`** (inside `ArchiveMetadata.header`):
  - `magic`: identifies the file as a Huffman folder archive.
  - `version`: format version.
  - `file_count`: number of file entries.
  - `total_original_size`: sum of uncompressed byte sizes across all files.
  - `total_compressed_size`: sum of stored/compressed sizes in the archive.
  - `header_size`: total size in bytes of the header and entries region.
- **`FileEntry`**:
  - `relative_path`: path of the file relative to the compressed folder root.
  - `original_size`, `compressed_size`.
  - `data_offset`: byte offset within the archive where this file's payload begins.
  - `timestamp`: last-modified time.
  - `checksum`: CRC32 of original data.
  - `is_compressed`: `true` if Huffman-compressed, `false` if stored verbatim.

## Core Methods

### Collection and Path Handling
- `std::vector<std::string> collectFiles(const std::string& folder_path)`:
  - Validates that `folder_path` exists and is a directory.
  - Recursively walks using `std::filesystem::recursive_directory_iterator`.
  - Collects and sorts file paths for deterministic ordering.
- `std::string makeRelativePath(const std::string& base_path, const std::string& full_path)`:
  - Computes a path relative to `base_path` using `fs::relative`.
  - Normalizes path separators to forward slashes for cross-platform archives.
- `void createDirectoryRecursive(const std::string& path)`:
  - Ensures parent directories exist when extracting files.

### Header I/O
- `bool writeArchiveHeader(std::ofstream& out, const ArchiveMetadata& metadata)`:
  - Writes the fixed-size header fields.
  - Iterates all `FileEntry`s and writes:
    - Path length (`uint64_t`) + path bytes.
    - `original_size`, `compressed_size`, `data_offset`, `timestamp`, `checksum`.
    - Compression flag (`uint8_t`).
- `bool readArchiveHeader(std::ifstream& in, ArchiveMetadata& metadata)`:
  - Reads header, validates `magic`.
  - Reads all file entries into `metadata.files`.

### Per-File Compression
- `bool compressSingleFileToArchive(...)`:
  - Reads the target file into memory (`original_data`).
  - Computes `original_size` and `checksum`.
  - Extracts filesystem last-write time to `timestamp`.
  - Calls `compressBuffer(original_data, settings)` from `HuffmanCompressor` to get `compressed_data`.
  - Decides whether to use compressed vs stored mode:
    - If compressed data exists and is **at least 10% smaller** than original, it writes the compressed buffer.
    - Otherwise, stores the original bytes uncompressed (`is_compressed = false`).
  - Sets `data_offset` to the current archive stream position, writes the chosen payload, and updates size fields.

### Per-File Decompression
- `bool decompressSingleFileFromArchive(std::ifstream& archive_stream, const FileEntry& entry, const std::string& output_path)`:
  - Seeks to `entry.data_offset` and reads `entry.compressed_size` bytes.
  - If `entry.is_compressed` is true, calls `decompressBuffer(file_data)`; otherwise uses raw data.
  - (Note: strict CRC32 validation has been intentionally removed to avoid failing whole archives on minor mismatches.)
  - Ensures directories for `output_path` exist and writes the reconstructed data.

### High-Level Archive Operations
- `bool compressFolder(const std::string& folder_path, const std::string& archive_path, const CompressionSettings& settings)`:
  - Collects files and creates `ArchiveMetadata` with initial `FileEntry`s.
  - Computes `header_size` via `metadata.calculateHeaderSize()`.
  - Opens `archive_path` and writes a placeholder block of `header_size` zero bytes.
  - Iterates files:
    - Optionally calls `progress_callback_` with `(index, total, relative_path)`.
    - Calls `compressSingleFileToArchive` for each.
    - Accumulates `total_original` and `total_compressed`.
  - Updates `header` totals and seeks back to write the real header over the placeholder.
  - Triggers final `progress_callback_` with `"Complete"`.
- `bool decompressArchive(const std::string& archive_path, const std::string& output_folder)`:
  - Opens archive and reads metadata.
  - Creates the output directory tree.
  - Iterates all `FileEntry`s, calling `decompressSingleFileFromArchive` for each.
  - Reports progress and returns success/failure.

### Introspection Helpers
- `ArchiveMetadata getArchiveInfo(const std::string& archive_path)`: Reads and returns only header + entries without extracting.
- `bool isValidArchive(const std::string& archive_path)`: Quick magic check.
- `std::vector<std::string> listArchiveFiles(const std::string& archive_path)`: Returns all relative paths stored in the archive.

## Interaction with Other Components
- **`HuffmanCompressor`**: Supplies `compressBuffer` / `decompressBuffer` and CRC32 logic.
- **CLI (`main_cli.cpp`) and API (`api_server.cpp`)**: Use `FolderCompressor` to implement folder-level commands and REST endpoints.
- **Filesystem**: Uses `std::filesystem` extensively for walking directories and managing paths.
