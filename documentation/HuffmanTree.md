# HuffmanTree.cpp Documentation

## Overview
`HuffmanTree` represents a binary Huffman coding tree and provides utilities to:

- Build a tree from symbol frequency counts.
- Derive canonical Huffman codes and code lengths.
- Serialize/deserialize tree structure.
- Export a Graphviz DOT representation of the tree.

This file implements the core algorithms around tree building, canonicalization, and visualization.

## Building the Tree
- `void HuffmanTree::build(const unordered_map<unsigned char, uint64_t>& freq)`:
  - Uses a `std::priority_queue` (min-heap) of `shared_ptr<HuffmanNode>` ordered by frequency, then by symbol value to break ties.
  - For each `(symbol, frequency)` pair, pushes a leaf node.
  - Repeatedly pops two lowest-frequency nodes and merges them into a parent whose `freq` is the sum and whose `byte` is `min(left->byte, right->byte)` for deterministic structure.
  - Continues until a single root node remains; this becomes `root`.

## Code Table Generation
- `HuffmanTree::CodeTable HuffmanTree::getCodes() const`:
  - Recursively traverses the tree via `buildCodes`.
- `void buildCodes(const shared_ptr<HuffmanNode>& node, const string& prefix, CodeTable& table) const`:
  - For leaves, assigns `prefix` as the code; uses "0" if the tree has a single symbol.
  - For internal nodes, recurses left with `prefix + "0"` and right with `prefix + "1"`.

## Serialization Support
- `vector<unsigned char> HuffmanTree::serialize() const` and helper `serializeNode`:
  - Performs a pre-order traversal.
  - For leaves: writes marker `1` followed by the symbol byte.
  - For internal nodes: writes marker `0` then recurses into left and right.
- `void HuffmanTree::deserialize(const vector<unsigned char>& data)` and helper `deserializeNode`:
  - Reads markers from `data` and reconstructs the tree.
  - Uses `parent_byte = min(left->byte, right->byte)` for deterministic internal-node bytes.
  - Throws runtime errors on malformed encodings (e.g., unexpected end of data or invalid marker).

## DOT Export
- `static std::string escapeLabel(unsigned char c)`:
  - Escapes special characters for Graphviz labels:
    - Backslash (`\\`) and double-quote (`\"`) are escaped explicitly.
    - Printable ASCII (32–126) is emitted as-is.
    - Non-printable bytes are rendered as `\xHH` hex escape sequences.
- `static void toDotNode(const shared_ptr<HuffmanNode>& node, std::ostream& out, std::unordered_map<const HuffmanNode*, int>& ids, int& idCounter)`:
  - Assigns stable node IDs and emits one `nodeN` definition per tree node.
  - Leaf nodes label: `"<escaped-symbol>\nFreq: <freq>"`.
  - Internal nodes label: `"Freq: <freq>"`.
  - Emits edges from parent to left (`label="0"`) and right (`label="1"`).
- `std::string HuffmanTree::toDot() const`:
  - Wraps tree export into a `digraph HuffmanTree { ... }` DOT graph.

## Canonical Code Derivation
- `HuffmanTree::CodeLenTable getCodeLengths() const`:
  - Computes code lengths by generating the raw code table via `getCodes()` and recording each length.
- `HuffmanTree::CodeTable getCanonicalCodes() const`:
  - Builds a vector of `(symbol, length)` pairs.
  - Sorts by `length`, then by `symbol`.
  - Implements canonical Huffman assignment:
    - Initialize `code = 0`, `prev_len = first.length`.
    - For each symbol:
      - If not the first: `++code`; if `len > prev_len`, left-shift by `len - prev_len`.
      - Convert `code` into a bitstring of length `len` (MSB first).
    - Stores resulting strings into the `CodeTable`.

## Usage in the Project
- `Compressor` uses `HuffmanTree::build`, `getCodeLengths`, and `getCanonicalCodes` to create efficient codebooks.
- `Decompressor` mirrors the canonical assignment algorithm to reconstruct codes from stored lengths.
- Both the CLI (`main_cli.cpp`) and API server (`api_server.cpp`) rely on `toDot()` to generate Huffman tree visualizations as DOT files.
