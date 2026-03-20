# Changelog

## Unreleased

- revived the CMake-based CLI build as the active project path
- validated Windows and Linux build + smoke coverage
- added fixture-based smoke tests and mixed-batch partial-failure coverage
- improved per-file failure handling so bad input no longer aborts the whole batch path
- strengthened corruption probing with a deterministic harness and oracle-backed decoder baseline
- documented release packaging, validation status, and contributor workflow for the revival branch
- generated a Linux Phase 4 dev package from the current workspace
- documented the current Windows packaging blocker in this workspace snapshot
