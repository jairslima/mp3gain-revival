# Release Notes

This project is not yet at a final public release milestone, but the current
technical release shape is already clear enough to document.

## Windows Package

- Required files:
  - `mp3gain.exe`
  - `mpg123.dll`
  - `LICENSE`
- Recommended source location:
  - the corresponding Git revision or source archive for the release

## Linux Package

- Required files:
  - `mp3gain`
  - `LICENSE`
- Prefer documenting:
  - distro/environment used to build
  - compiler version
  - `libmpg123` version used during validation

## Pre-Release Checklist

1. Build the active CMake target successfully.
2. Run `bash test/smoke_test.sh` and confirm the suite passes.
3. Run `pwsh -File test\probe_corruptions.ps1` and archive the current results matrix.
4. Confirm the Windows package contains `mpg123.dll`.
5. Keep LGPL notices and corresponding source availability explicit.
6. Summarize known limitations, especially remaining legacy global-state coupling and the current decoder mismatch against the oracle-backed corruption baseline.

## Validation Snapshot

- Windows: build + smoke validated
- Linux: build + smoke validated
- macOS: outside the immediate release support baseline
- Current smoke baseline: `27/27`
- Current corruption probe baseline: truncated candidate fails in MP3Gain; multiple additional stable candidates trigger real decoder errors in the `ffmpeg` oracle while still analyzing successfully in MP3Gain/libmpg123
- See also: `docs/VALIDATION.md`

## Current Release Blockers

- remaining legacy bitstream/write-context coupling in `project/mp3gain.c`
- packaging, notes, and final artifact curation for the first public release

## Phase 4 Gate

Move from Phase 3 to Phase 4 only when all of the following are true:

1. `bash test/smoke_test.sh` remains green on the active supported platforms.
2. `pwsh -File test\probe_corruptions.ps1` remains green and preserves the current oracle-backed corruption baseline.
3. Windows and Linux package shapes are reproducible from the documented build path.
4. macOS remains outside the near-term supported-platform claim unless validated separately.
5. The current blockers list can be reduced to an explicit release stance on remaining limitations rather than unmeasured verification gaps.

## Release Handoff Status

- checklist: ready
- package shape: ready for Windows and Linux
- supported-platform statement: ready
- blockers list: explicit
- Phase 4 gate: explicit
- remaining reason not to declare release-readiness today: Phase 4 packaging/release execution work is still pending

## Current Packaging Status

- Linux package can already be produced in this workspace with `pwsh -File scripts/package_release.ps1 -Platform linux -Version <tag>`
- Linux package has been generated successfully in this workspace as `dist/mp3gain-linux-phase4-dev.zip`
- Windows package requires `build/Release/mp3gain.exe` and `build/Release/mpg123.dll`; those artifacts are not present in the current workspace snapshot
- the current PowerShell environment also does not expose local `cmake`/MSBuild tooling directly, so Windows packaging remains blocked here until a Windows build snapshot is prepared
- latest Windows packaging attempt fails immediately with `Missing Windows executable: build/Release/mp3gain.exe`

## Supported Platform Statement

- Current supported validation baseline for release decisions: Windows and Linux
- macOS is explicitly outside the immediate support claim for the first public recovery release
- Do not claim macOS support in a release until build and smoke validation have both been executed there
- The MP3Gain/libmpg123 vs `ffmpeg` corruption mismatch is accepted as a known limitation for the first public recovery release

## Suggested Release Bundle

### Windows
- `build/Release/mp3gain.exe`
- `build/Release/mpg123.dll`
- `LICENSE`
- short release notes mentioning the exact Git revision
- optional helper: `pwsh -File scripts/package_release.ps1 -Platform windows -Version <tag>`

### Linux
- `build/mp3gain`
- `LICENSE`
- short release notes mentioning distro, compiler, and `libmpg123` version
- optional helper: `pwsh -File scripts/package_release.ps1 -Platform linux -Version <tag>`

## Suggested Release Notes Topics

- verification baseline used for the release, including smoke-test count
- supported packaging targets for the release
- remaining limitations:
  - legacy global/shared state still being reduced
  - MP3Gain/libmpg123 still tolerates some corruption cases that the `ffmpeg` oracle flags as invalid
  - GUI source is not part of this repository
