# Architecture Plan

This document describes the intended technical direction for the MP3Gain revival fork.

## Current State

The active source tree in `project/` is a legacy C codebase centered around a large monolithic executable entry point in `mp3gain.c`.

The code currently mixes several concerns in the same compilation unit:
- command-line parsing
- file traversal and file mutation
- MPEG frame parsing and gain modification
- decoding through `libmpg123`
- ReplayGain analysis
- ID3 and APE tag read/write logic
- platform-specific behavior
- old DLL compatibility paths

That layout makes the code difficult to test, port, and simplify.

## Existing Functional Areas

### 1. ReplayGain analysis core
- Files: `gain_analysis.c`, `gain_analysis.h`
- Status: relatively self-contained
- Direction: preserve as the basis of the portable analysis core

### 2. Tag handling
- Files: `id3tag.c`, `id3tag.h`, `apetag.c`, `apetag.h`
- Status: useful but coupled to direct file I/O and legacy conventions
- Direction: keep functionality, but isolate behind a smaller interface

### 3. Processing engine
- File: `mp3gain.c`
- Status: overloaded and monolithic
- Direction: split into smaller modules with explicit responsibilities

Current extraction status:
- `cli.[ch]`: argument parsing and table-header output
- `cli.[ch]`: argument parsing, option application, and table-header output
- `prep.[ch]`: per-file preparation and recalc setup
- `exec.[ch]`: simple execution branches that do not enter the heavy analysis path
- `process.[ch]`: file opening, decoder setup, first-frame MP3 prep, result finalization, and apply/report helpers
- `process.[ch]` also now owns runtime allocation/setup batching, including `fileok` allocation, initial file-prepare batching, album recalc computation, the per-file batch loop, the per-file recalc path, the post-track-finalization path, the album post-processing path, final batch-finalization/cleanup, and the full MP3 frame-processing loop, including per-frame header parsing, per-frame decode/analyze, frame-advance/progress helpers, and the protected per-frame execution wrapper
- `mp3gain.c`: still owns the most sensitive frame-by-frame analysis path and top-level orchestration

### 4. Legacy Windows DLL support
- Files: `replaygaindll.c`, `replaygaindll.def`, DLL-related code paths in headers and sources
- Status: historical compatibility layer
- Direction: remove from the main recovery path, then decide whether to archive or maintain separately

### 5. Historical build definitions
- Files: `Makefile`, `.sln`, `.vcproj`, `.dsp`, `.dsw`
- Status: reference only
- Direction: preserve as historical material, but do not treat them as the supported build path

## Target Architecture

The long-term target is a small portable core with a thin CLI wrapper.

Suggested module boundaries:

### `core/analysis`
- ReplayGain calculations
- no CLI output
- no direct filesystem access

### `core/decoder`
- adapter around `libmpg123`
- stable internal interface for decoded PCM samples
- one place to handle decoder-specific quirks

### `core/mp3`
- frame parsing
- gain application logic
- clipping and min/max calculations

### `core/tags`
- tag read/write operations
- normalization of ID3 and APE handling behind one internal API

### `core/fs`
- portable file replacement, timestamp preservation, temp-file handling
- remove scattered platform conditionals from business logic

### `cli`
- parse arguments
- invoke engine operations
- render results and diagnostics

## Refactoring Sequence

### Phase A: Build recovery
- make the current CMake target compile on one platform
- confirm a reproducible dependency path for `libmpg123`
- avoid semantic changes except where required for build correctness

### Phase B: Structural extraction
- split `mp3gain.c` into smaller translation units without changing behavior
- isolate CLI parsing from processing code
- isolate `libmpg123` usage behind a dedicated adapter
- isolate file and timestamp operations behind helper functions

Progress so far:
- CLI parsing is already isolated
- most non-loop orchestration around per-file processing is already isolated
- album/track reporting and apply paths are already isolated
- the next meaningful extraction boundary is the `while (ok)` frame-analysis loop

### Phase C: Legacy reduction
- remove or disable DLL-specific recovery-path code from the default build
- keep DLL sources as archived or optional components only
- stop depending on obsolete Visual Studio project assumptions

### Phase D: Verification
- add smoke tests for analysis-only flows
- add fixture-based tests for tag read/write behavior
- add regression tests for undo, clipping, and album gain logic

### Phase E: Cross-platform support
- validate Windows, Linux, and macOS builds in CI
- document supported toolchains
- keep platform-specific code behind narrow interfaces

## Immediate Engineering Priorities

1. Make the CMake build reliable.
2. Finish extracting the remaining frame-analysis loop from `mp3gain.c`.
3. Remove DLL concerns from the main executable path.
4. Define a stable internal boundary between decoder, tag handling, and gain logic.
5. Add minimal tests before deeper behavior changes.

## Non-Goals For Early Phases

- recovering the historical GUI before the CLI is stable
- preserving obsolete build systems as supported paths
- redesigning file formats or changing ReplayGain behavior without regression coverage
