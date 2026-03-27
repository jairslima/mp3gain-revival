# Release Notes: 1.6.2-revival2

The first public recovery release of MP3Gain Revival.

## Scope
This release targets the supported baseline:
- Windows
- Linux

*(macOS is not part of the first public recovery release claim)*

## Verification Baseline
- Build validated on Windows and Linux.
- Smoke suite baseline: `27/27` tests passing.
- Corruption probe baseline persisted in `test/probe_corruptions.results.json`.

## Release Notes
- Revives the MP3Gain CLI build on a modern CMake path.
- Validates Windows and Linux as the first supported release baseline.
- Adds fixture-based smoke coverage for analysis, apply, undo, tags, clipping, and mixed valid/invalid batches.
- Improves per-file failure handling so bad inputs no longer abort the whole active batch path.
- Documents corruption probing with a deterministic oracle-backed baseline.

## Known Limitations
- macOS is not yet in the supported release baseline.
- Legacy low-level shared state still exists in `project/mp3gain.c`.
- Some corrupted MP3s that `ffmpeg` flags as decode-invalid are still tolerated by the current MP3Gain/libmpg123 path.
- Release publication still needs to be executed explicitly for each snapshot even when locally validated package zips already exist.

## Packaging
- **Windows Bundle**: `mp3gain.exe`, `mpg123.dll`, and `LICENSE`.
- **Linux Bundle**: `mp3gain` binary and `LICENSE`.
