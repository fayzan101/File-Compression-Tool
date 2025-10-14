# Progress — HuffmanCompressor

Overview

This document tracks the work done on the HuffmanCompressor scaffold and the next concrete steps to complete a working tool.

Completed

- Project scaffold created (include/, src/, CMakeLists.txt)
 - Project scaffold created (include/, src/, run.bat)
- Header files added for core components: HuffmanNode, HuffmanTree, BitWriter, BitReader, Compressor, Decompressor
- `using namespace std;` added to headers per request (note: this pollutes global namespace)
- `data/`, `docs/`, and `tests/` folders added with README placeholders

Current in-progress

- Implement minimal, compile-ready stubs (source files) in `src/` and `main.cpp` to allow building the project

Next concrete todos (detailed)

Below are concrete, ordered tasks with small actionable substeps, target files, rough time estimates, priority, and acceptance criteria. Follow each substep, commit often, and open a PR for review when a task completes.

1) Implement `BitWriter` and `BitReader`
   - Purpose: provide bit-level write/read utilities used by compressor/decompressor.
   - Files to add/modify: `src/BitWriter.cpp`, `src/BitReader.cpp`, update `include/BitWriter.h`, `include/BitReader.h` if necessary.
   - Substeps:
     1. Create `BitWriter::writeBit(bool)` and `BitWriter::writeBits(uint64_t value, unsigned count)` that buffer bits to a byte buffer.
     2. Implement `BitWriter::flush()` to write any remaining bits (pad with zeros) and expose a method to retrieve the written bytes (or write directly to an ostream/file).
     3. Create `BitReader::readBit()` and `BitReader::readBits(unsigned count)` to consume bits from a byte buffer/istream.
     4. Add boundary checks and clear documentation comments for byte/bit ordering (LSB-first or MSB-first). Choose MSB-first unless repo conventions differ.
     5. Write a unit test `tests/bit_io_test.cpp` that writes a known sequence of bits, flushes, reads back, and asserts equality.
   - Estimate: 2–4 hours.
   - Priority: High.
   - Acceptance criteria: Tests pass; behavior deterministic for known patterns; code compiles without modifying other modules.

2) Implement `HuffmanTree` core functionality
   - Purpose: build Huffman tree from symbol frequencies and produce a code table.
   - Files to add/modify: `src/HuffmanTree.cpp`, possibly `include/HuffmanTree.h` (ensure public API is stable for other modules).
   - Substeps:
     1. Implement a method `build(const std::unordered_map<uint8_t, uint64_t>& freqs)` that creates tree nodes and computes codes.
     2. Implement `getCodeTable()` returning `std::unordered_map<uint8_t, std::string>` (or a vector of bit sequences) representing codewords.
     3. Implement a simple serialize/deserialize pair for the tree or code table. For now, a header that stores symbol->code-length and code bits as a compact map is sufficient.
     4. Add unit tests `tests/huffman_tree_test.cpp` verifying codes for a small frequency table and checking the prefix-free property (no code is a prefix of another).
   - Estimate: 3–6 hours.
   - Priority: High.
   - Acceptance criteria: Given a deterministic frequency map, the produced code table is prefix-free and stable across runs (or reproducible via deterministic tie-breaking).

3) Implement `Compressor` and `Decompressor`
   - Purpose: provide end-to-end compress/decompress flow, writing a compact header and encoded payload.
   - Files to add/modify: `src/Compressor.cpp`, `src/Decompressor.cpp`, `include/Compressor.h`, `include/Decompressor.h` as needed.
   - Substeps — Compressor:
     1. Read an input file as bytes and count frequencies into `std::unordered_map<uint8_t, uint64_t>`.
     2. Use `HuffmanTree::build()` to create codes.
     3. Write a header containing a magic number (e.g., "HUF1"), original file size, and a serialized code table. Keep the header small and well-documented.
     4. Encode the input bytes using `BitWriter` and write the bitstream after the header.
     5. Provide CLI options for input/output paths and a `--test` or `--no-header` flag for debugging.
   - Substeps — Decompressor:
     1. Read header and validate the magic number/version.
     2. Deserialize the code table and construct a decoding structure (a tree or table).
     3. Use `BitReader` to decode the bitstream and write the original bytes to the output file.
   - Tests:
     - `tests/integration_small.cpp`: compress a small sample file in `data/` then decompress and assert equality with the original.
   - Estimate: 4–8 hours.
   - Priority: High.
   - Acceptance criteria: Integration test passes; produced compressed file contains expected header and decompresses to the original byte-for-byte.

4) Add a minimal CLI entry point and `run.bat` build wrapper
  - Purpose: allow building and running the compressor from command line via `run.bat` on Windows.
  - Files to add/modify: `src/main.cpp`. Add or update `run.bat` (project root) to configure/build/run tests.
   - Substeps:
     1. Implement `main.cpp` with simple argument parsing (positional: `compress|decompress`, `input`, `output`). Keep parsing minimal — add a TODO to replace with a proper CLI parser later.
  2. Ensure the build configuration produces an executable target `huffman` linking the library sources; `run.bat build` will invoke the configured build system (CMake or other) to produce the executable.
     3. Document the build and run steps briefly in `progress.md` or `README.md`.
   - Estimate: 1–2 hours.
   - Priority: Medium.
   - Acceptance criteria: Project builds and `huffman --help` (or `huffman compress`) runs and returns expected exit codes for invalid arguments.

5) Build, verify, and add tests runner
   - Purpose: ensure code compiles on Windows and tests run automatically.
   - Substeps:
  1. Run `run.bat build` from PowerShell and fix any compile errors.
  2. Add a simple test runner or ensure tests run via `run.bat test` (this will call `ctest` if available after building).
  3. Add a short CI-style checklist in `progress.md` for Windows builds (how to use `run.bat`, Visual Studio/MSVC or MinGW notes).
   - Estimate: 0.5–2 hours (plus time to fix compile issues).
   - Priority: High.

Medium-term enhancements (after basic correctness)

- Implement canonical Huffman codes to reduce header size and ensure decoder independence from tie-breaking.
- Add streaming APIs to compress very large files without loading everything into memory.
- Improve header format to include checksums, block-based compression, and versioning.

Quick acceptance checklist (before merging any of the above):

- Code compiles cleanly on Windows using `run.bat` (which wraps the project's build commands, e.g., CMake).
- Unit tests for `BitReader/BitWriter` and `HuffmanTree` pass locally.
- Integration test compress->decompress yields original file.
- Header format documented in `docs/` or `progress.md` (at least the basics: magic, version, table format).

How to prioritize and estimate time

- Start with `BitWriter`/`BitReader` then `HuffmanTree` — both are foundational. Tackle `Compressor`/`Decompressor` next.
- Break each main task into commits: implementation, tests, fixups.

Notes and assumptions

- Assumed symbol alphabet is raw bytes (0..255). If project uses a different alphabet, adjust frequency data structures.
- Use MSB-first bit ordering in the bitstream unless other code indicates otherwise. Document the choice in the header comments.
- Keep header serialization simple to start; canonicalization and compression of the table are later improvements.

Next immediate action for me (if you'd like): I can implement `BitWriter`/`BitReader` and the unit test scaffolding next and run the build. Indicate if you prefer me to proceed.

Compression Levels (Fast..Best and gzip-like -1..-9)

Goal: offer user-selectable compression modes that trade CPU/time for compression ratio. Provide both simple named modes (Fast, Default, Best) and numeric `-1`..`-9` flags mirroring gzip semantics so users can choose precisely.

Design & mapping (concrete)

- Modes: `fast`, `default`, `best` (also accept `-1` .. `-9` numeric levels).
- Mapping strategy: map numeric levels to algorithmic choices controlling two orthogonal knobs:
  1. Tree optimization effort: how aggressively we optimize codes (e.g., basic Huffman vs. canonicalizing and optimizing for code-length distributions).
  2. Block/streaming strategy and sampling: whether to analyze the whole file (better compression) or use block-based/streaming heuristics (faster, less memory).

Suggested concrete mapping:

- Levels 1..3 (Fast):
  - Behavior: single-pass, streaming-friendly. Build frequency counts using fixed-size blocks (e.g., 64 KiB) or 1-pass approximate counting. Use basic Huffman code generation with minimal tie-breaking work.
  - Parameters: block_size = 64 KiB, allow_sampling = true, optimize_tree = false, prefer_speed = true.
  - Example flags: `--level 1` or `-1` or `--mode fast`.

- Levels 4..6 (Default / Balanced):
  - Behavior: read whole file to compute exact frequencies (if file fits memory) or aggregate block counts for large files. Build full Huffman tree and perform canonicalization for stable code assignment.
  - Parameters: block_size = file_size (or 1 MiB chunks for huge files), allow_sampling = false, optimize_tree = true, prefer_speed = false.
  - Example flags: `--level 5` or `-5` or `--mode default`.

- Levels 7..9 (Best):
  - Behavior: multi-pass and heavier tree/code optimizations. Consider experimenting with augmentations such as run-length modeling or using larger context models for highly compressible inputs. Use full canonical Huffman plus optional extra passes to rebalance code lengths.
  - Parameters: block_size = file_size, allow_sampling = false, optimize_tree = true, extra_passes = 1..2, prefer_size = true.
  - Example flags: `--level 9` or `-9` or `--mode best`.

Concrete implementation steps

1. Define a `CompressionSettings` struct
  - File: `include/Compressor.h` (or a new `include/CompressionSettings.h`)
  - Fields: `unsigned level` (1..9), `enum Mode { FAST, DEFAULT, BEST }`, `size_t block_size`, `bool canonicalize`, `unsigned extra_passes`, `bool sampling`, `bool prefer_speed`.
  - Provide a helper `CompressionSettings make_settings_from_level(unsigned level)` that maps levels to parameter sets above.

2. CLI parsing and flags
  - File: `src/main.cpp`
  - Add flags: `-1`..`-9` (numeric shortcuts) and `--level N`, plus `--mode fast|default|best`.
  - Parsing precedence: explicit `--level N` overrides `--mode`. If both absent, default to level 5 / mode default.

3. Wire settings into `Compressor`
  - File: `src/Compressor.cpp`, `include/Compressor.h`
  - Add an API `Compressor::compress(const CompressionSettings& settings, ...)` or pass settings to existing methods.
  - Use `settings.block_size` and `settings.sampling` to decide whether to read the full file or use block-based frequency aggregation.
  - Use `settings.canonicalize` and `settings.extra_passes` to decide whether to canonicalize and/or re-optimize code tables.

4. Tests and benchmarks
  - Files: `tests/level_mapping_test.cpp`, `tests/level_integration_test.cpp`.
  - Unit test `level_mapping_test.cpp`: assert `make_settings_from_level()` produces expected parameter values for representative levels (1,5,9).
  - Integration test `level_integration_test.cpp`: compress a sample file with `-1`, `-5`, `-9`, measure compressed sizes, and assert size(-9) <= size(-5) <= size(-1) (allow small tolerances); also verify decompress correctness.

5. Documentation
  - Add a short section in `progress.md` (this paragraph) and `docs/README.md` describing the levels, their effects, and examples for CLI usage.

Estimates & priority

- Design and settings mapping: 1–2 hours.
- CLI parsing and wiring to `Compressor`: 1–3 hours.
- Tests and benchmarks: 1–3 hours.
- Docs update: 0.5 hours.

Acceptance criteria

- `--level` / `-N` and `--mode` flags accepted by `main` and correctly parsed.
- `make_settings_from_level()` passes unit tests.
- Running compress->decompress with different levels yields valid outputs; higher levels produce equal or smaller compressed outputs in integration tests.

Notes

- The mapping above is intentionally coarse and conservative; exact parameter tuning (block sizes, number of extra passes) can be refined after profiling real inputs.
- For very large files, prefer block-based aggregation even for higher levels, but allow an option `--in-memory` to force whole-file analysis when memory allows.

If you'd like, I can implement `CompressionSettings`, the CLI flags, and the `level_mapping_test.cpp` next and run the tests. Say which parts you want implemented first and I'll proceed.

5) Compression Stats by File Type
   - Purpose: quantify how compression level choices affect compression ratio and speed across common file types (text, source code, JSON, XML, CSV, images, audio, binaries).
   - Files to add/modify: `tools/benchmark.cpp` (small harness), `tests/level_integration_test.cpp` (extend), `data/samples/` (place representative sample files), `docs/stats/README.md` (report + CSVs).
   - Substeps:
     1. Assemble sample files: collect small-to-medium example files for common types and place them under `data/samples/` (e.g., `text/sample.txt`, `json/sample.json`, `png/sample.png`, `bin/sample.bin`). If you prefer, start with a minimal set (text, JSON, PNG, binary) and expand later.
     2. Create `tools/benchmark.cpp`: a small program or test that runs the compressor for a list of files and levels (`-1`, `-5`, `-9`) and records: original size, compressed size, compression ratio, compression time (wall-clock), and peak memory (optional).
     3. Output results as CSV/JSON under `docs/stats/results-YYYYMMDD.csv` and a short summary `docs/stats/summary.md` that highlights trends and recommendations.
     4. Add a reproducible command in `README.md` or `docs/stats/README.md` showing how to run the benchmark on Windows PowerShell and how to extend the sample set.
     5. Add an integration test `tests/level_integration_test.cpp` that verifies for each sample file: decompress(compress(file, level)) == original; and optionally asserts size(-9) <= size(-5) <= size(-1) within a small tolerance.
   - Metrics to collect: original_size, compressed_size, compression_ratio, time_ms, memory_kib (if available), level, file_type, file_name.
   - Estimate: 24 hours depending on the breadth of sample files and whether peak memory measurement is implemented.
   - Priority: Medium (high value for documentation and tuning).
   - Acceptance criteria:
     - The benchmark harness runs and produces CSV results for a representative set of files.
     - Integration tests for correctness pass for each level.
     - A short human-readable report `docs/stats/summary.md` is present that summarizes findings and recommended defaults.

Notes

 - Start small: pick 4-8 representative samples and run the benchmark on those. Expand sample corpus over time.
 - Peak memory measurements are platform-dependent; include them as optional columns in the CSV and document the method used (e.g., platform-specific APIs or external tools).

If you'd like, I can scaffold `tools/benchmark.cpp`, add a small sample set under `data/samples/`, and run one quick benchmark run with levels `1,5,9` to produce a demonstration CSV. Approve and I'll proceed.

9) Library Mode (C++ API)
   - Purpose: expose the compressor/decompressor as a reusable C++ library so other projects can embed it (both static and shared builds), and provide a clear public API and examples.
  - Files to add/modify: `include/` (public API headers), `src/` (library entry points), `examples/` (small example programs showing API usage), `run.bat`/build scripts (make sure `run.bat` can build library targets), `docs/` (usage docs).
   - Design contract (short):
     - Inputs: raw byte buffers or input streams, `CompressionSettings` (level, block_size, flags), output stream or callback for compressed bytes.
     - Outputs: compressed byte stream (to ostream or std::vector<uint8_t>), status/error codes, optionally metadata (original size, compressed size, header info).
     - Error modes: report via return codes or exceptions (choose one consistent policy project-wide). Document both behaviors.
   - Suggested public API (example)
     - Header: `include/HuffmanCompressor.h`
       - Namespace: `huffman` or `huffman::v1`.
       - Types:
         - `struct CompressionSettings { unsigned level; size_t block_size; bool canonicalize; unsigned extra_passes; /*...*/ }`;
         - `struct CompressionResult { size_t original_size; size_t compressed_size; bool success; std::string error; }`;
       - Functions / classes:
         - `CompressionResult compress(std::istream& in, std::ostream& out, const CompressionSettings& settings);`
         - `CompressionResult decompress(std::istream& in, std::ostream& out);`
         - `std::vector<uint8_t> compressBuffer(const std::vector<uint8_t>& in, const CompressionSettings& settings);`
         - `std::vector<uint8_t> decompressBuffer(const std::vector<uint8_t>& in);`

   - Concrete implementation steps
     1. Finalize the `CompressionSettings` and `CompressionResult` types in `include/HuffmanCompressor.h` and document them.
     2. Implement thin wrappers in `src/library.cpp` that adapt existing `Compressor`/`Decompressor` code paths to the public API (streams and buffer helpers).
    3. Ensure a library target exists in your build system (CMakeLists.txt or equivalent). Example with CMake:
      - `add_library(huffman STATIC ${SRC_FILES})`
      - Optionally `add_library(huffman_shared SHARED ...)` or use `BUILD_SHARED_LIBS` configuration.
      - Install/export targets and add `target_include_directories(huffman PUBLIC ${PROJECT_SOURCE_DIR}/include)`.
      - Update `run.bat` so `run.bat build` configures and builds these targets.
     4. Add `examples/` with two small programs: `examples/compress_file.cpp` and `examples/in_memory.cpp` demonstrating file and buffer APIs.
     5. Add unit tests for the buffer APIs (`tests/library_api_test.cpp`) that call `compressBuffer` and `decompressBuffer` and assert round-trip equality.
     6. Add a short `docs/library.md` describing the API, threading/ownership rules, and linking instructions.

   - Packaging & portability notes
  - Build both static and shared variants if convenient. Provide options (via `run.bat`) such as `-DBUILD_SHARED_LIBS=ON` and `-DHUFFMAN_INSTALL=ON` that will be passed to the underlying build system.
     - Consider semantic versioned namespace (e.g., `huffman::v1`) if API stability and ABI compatibility are concerns.

   - Estimates & priority
     - API design and headers: 1 hour.
  - Library wrappers and build targets: 1–2 hours.
     - Examples and tests: 1–2 hours.
     - Docs and packaging notes: 0.5–1 hour.
     - Priority: High (makes the code reusable).

   - Acceptance criteria
  - Library target builds (static/shared) via `run.bat` on Windows (which invokes the configured build system).
     - Example programs compile and demonstrate `compressBuffer`/`decompressBuffer` and stream-based calls.
     - Unit tests for the library API pass.

Notes

 - Keep the public API minimal and stable. Expose higher-level convenience functions (buffer and stream helpers) and keep complex internals internal to `src/`.
 - Decide early whether the library will use exceptions or return error codes; document the choice in `docs/library.md`.

If you'd like, I can scaffold `include/HuffmanCompressor.h`, `src/library.cpp`, update build scripts or `CMakeLists.txt` with a `huffman` library target, add `examples/` and tests, and update `run.bat`. Approve and I'll implement those next and run a build on Windows.
