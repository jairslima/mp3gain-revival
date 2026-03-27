# Changelog

## [1.6.2-revival2] - 2026-03-27

- revived the CMake-based CLI build as the active project path
- validated Windows and Linux build + smoke coverage
- added fixture-based smoke tests and mixed-batch partial-failure coverage
- improved per-file failure handling so bad input no longer aborts the whole batch path
- strengthened corruption probing with a deterministic harness and oracle-backed decoder baseline
- documented release packaging, validation status, and contributor workflow for the revival branch
- generated Linux and Windows Phase 4 dev packages from the current workspace
- corrected mixed `bash`/WSL validation paths so the smoke suite measures the real Windows binary when selected
- corrected the corruption probe so it detects an actual available Linux test binary instead of relying on a stale hardcoded path
- aligned the Avalonia GUI target loudness baseline with the current `95.5 dB` ReplayGain reference
