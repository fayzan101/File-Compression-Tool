# profiler.cpp Documentation

## Overview
`profiler.cpp` contains a small utility for querying the process peak memory usage (RSS: Resident Set Size) on Windows. It is used in conjunction with the `Profiler` interface declared in `Profiler.h`.

## Platform-Specific Behavior
- Includes `<windows.h>` and `<psapi.h>` to access Windows process APIs.
- Uses `GetCurrentProcess()` and `GetProcessMemoryInfo()`.

## Key Function
- `uint64_t util::getCurrentPeakRSS()`:
  - Calls `GetProcessMemoryInfo` to fill a `PROCESS_MEMORY_COUNTERS` structure.
  - Returns `pmc.PeakWorkingSetSize` as a 64-bit integer if successful.
  - Returns `0` if the information cannot be obtained.

## Usage in the Project
- Intended for profiling and tooling support to measure peak memory usage during compression/decompression runs.
- Can be wired into benchmarks or CLI options to report memory consumption.
