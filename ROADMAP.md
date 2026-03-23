# Roadmap

## Phase 1: Repository Recovery
- Preserve the recovered historical material
- Establish a clean active source tree
- Add repository-level documentation
- Define a modern build entry point
- Establish clear workspace hygiene so source inspection stays scoped to active code and docs
- Document the project explicitly as an LGPL fork/continuation of MP3Gain

## Phase 2: CLI Build Recovery
- Validate the CMake-based build on one supported platform
- Confirm the required `libmpg123` dependency path
- Resolve compiler and portability issues in the active CLI sources
- Document exact build commands for Windows and Unix-like systems
- Remove or isolate legacy build assumptions that block portability
- Start separating `mp3gain.c` into CLI, engine, decoder, and tag-handling responsibilities

Current Phase 2 progress:
- CLI parsing has been moved out of `mp3gain.c`
- non-loop file preparation and simple execution branches have been moved out of `mp3gain.c`
- decoder setup and result/apply helpers have been moved out of `mp3gain.c`
- the batch/runtime state now has a small `process.[ch]` context layer
- part of the frame-scan state access has been moved behind helpers in `mp3gain.c`
- the main remaining extraction target is the deeper frame-by-frame scan state and legacy write path

## Phase 3: Verification

### Phase 3.1: Baseline Smoke Coverage
- Add sample-based smoke tests for analysis mode
- Lock fixture-based verification for representative MP3 inputs
- Ensure the smoke suite can run repeatedly without mutating checked-in inputs
- Status: complete

Current state:
- smoke coverage exists for version, track analysis, album analysis, apply track gain, undo, apply album gain, APE tag round-trip, ID3/APE mode separation, auto-clip flow, and mixed valid/invalid batch handling
- reproducible fixtures now live in `test/fixtures/`

### Phase 3.2: Stateful CLI Behavior
- Verify tag read/write behavior on representative MP3 files
- Check undo behavior and clipping-related flows
- Confirm file mutation paths preserve expected behavior after refactors
- Status: complete

Current state:
- tag write/read, undo, apply, and auto-clip flows are covered in the smoke suite

### Phase 3.3: Partial-Failure and Batch Safety
- Cover mixed-success batch execution
- Replace process-wide aborts with per-file failure handling where safe
- Lock non-zero exit semantics for partial failures
- Status: complete

Sub-steps:
- 3.3.1 split monolithic write-path/error-path logic into smaller helpers
- 3.3.2 keep per-file failure handling explicit in active processing paths
- 3.3.3 reduce low-level shared write/bitstream state exposure
- 3.3.4 document which remaining fatal exits are acceptable CLI exits vs. true debt

Current state:
- partial-failure batch behavior is covered and no longer aborts the process on the first bad MP3 in the main processing path
- the mixed valid/invalid fixture path is enforced in `test/smoke_test.sh`
- the legacy write path is now being reduced through smaller helpers instead of large monolithic frame-mutation blocks
- the low-level bitstream path has been thinned further with shared side-info, channel/granule, and global-gain cursor helpers
- the remaining legacy low-level state is now a structural cleanup concern, not a blocker for batch safety semantics

### Phase 3.4: CI and Cross-Platform Build Validation
- Add continuous integration
- Stand up cross-platform validation for Windows, Linux, and macOS
- Keep the active CLI path building on supported targets
- Status: complete

Sub-steps:
- 3.4.1 keep an explicit validated-platform matrix in repo docs
- 3.4.2 keep CI coverage aligned with the active validated platforms
- 3.4.3 document local reproduction commands for validated platforms
- 3.4.4 keep supported-platform policy explicit, including pending platforms

Current state:
- Windows and Linux CI jobs exist and build/test the active CLI path
- the current supported-platform baseline is now explicitly Windows and Linux
- macOS remains exploratory and outside the near-term support claim until a validated baseline exists

### Phase 3.5: Corruption and Decoder Regression Coverage
- Introduce regression coverage before behavior-changing simplifications
- Add fixtures or probes for truly corrupted MPEG frame data
- Distinguish “missing valid frame” rejection from true decoder/frame corruption
- Status: complete

Sub-steps:
- 3.5.1 classify corruption modes worth probing
  - 3.5.1.1 frame discovery corruption
  - 3.5.1.2 payload corruption with header preservation
  - 3.5.1.3 header-window corruption after initial sync
  - 3.5.1.4 sync-loss corruption across later frames
  - 3.5.1.5 CRC/side-info corruption windows
  - 3.5.1.6 transition corruption between nominally valid frames
  - 3.5.1.7 reservoir-burst corruption inside later frames
  - 3.5.1.8 cross-frame-stitch corruption straddling frame boundaries
  - 3.5.1.9 side-info shear after initial sync
  - 3.5.1.10 ancillary/payload scramble after stable frame discovery
  - 3.5.1.11 consecutive later-frame header poisoning
  - 3.5.1.12 clustered multi-frame corruption after stable analysis startup
- 3.5.2 maintain a deterministic corruption probe harness
  - 3.5.2.1 implement and keep individual mutators deterministic
  - 3.5.2.2 persist the probe output matrix after each round
- 3.5.3 capture a repeatable baseline matrix of probe results
  - 3.5.3.1 compare exit-code behavior
  - 3.5.3.2 compare the first lines of stderr/stdout for failure shape
  - 3.5.3.3 compare against an external decoder oracle for true decode errors
- 3.5.4 promote a candidate into smoke/CI only if it is stable
  - 3.5.4.1 candidate must fail reproducibly
  - 3.5.4.2 candidate must reach decode/frame failure, not just missing-frame rejection
  - 3.5.4.3 if MP3Gain tolerates a stable corrupted candidate, keep it in the oracle-backed probe baseline instead of forcing a smoke expectation
- 3.5.5 document the remaining gap explicitly until a stable decode-failure fixture exists
  - 3.5.5.1 keep fixture docs aligned with the active matrix
  - 3.5.5.2 keep validation/release blockers aligned with the active matrix

Current state:
- missing-frame rejection is covered by `test/fixtures/corrupt-truncated.mp3`
- `test/probe_corruptions.ps1` now provides a deterministic probe harness for corruption candidates and records both MP3Gain behavior and an `ffmpeg` decoder oracle
- the oracle-backed matrix now shows true decoder errors for multiple stable corruption classes beyond missing-frame rejection
- the MP3Gain/libmpg123 path still tolerates several files that `ffmpeg` flags as decode-corrupt; that mismatch is now an explicit known limitation rather than a coverage gap
- `corrupt-truncated.mp3` remains the smoke/CI fixture for missing-frame rejection and partial-batch failure

### Phase 3.6: Verification-to-Release Handoff
- Stabilize the verification baseline used for release decisions
- Keep build, smoke, and contributor docs aligned with the current state
- Finish Phase 3 only when verification is strong enough to support public release packaging
- Status: complete

Sub-steps:
- 3.6.1 keep a release checklist aligned with the current verification baseline
- 3.6.2 define the package shape for currently validated platforms
- 3.6.3 keep release blockers explicit until the Phase 4 handoff is credible
- 3.6.4 define the concrete gate from Phase 3 into Phase 4

Current state:
- release docs now describe the package shape, validation baseline, and known limitations for the first public recovery release
- the checklist and package shape are now explicit for Windows and Linux
- the current decoder mismatch against the oracle-backed corruption probe is now treated as an explicit release limitation for the first public recovery release
- the Phase 4 handoff gate has been satisfied for the Windows/Linux supported baseline

## Phase 4: Release Readiness
- Write changelog and release notes for the revival branch
- Define supported platforms and toolchains
- Add packaging instructions
- Publish the first public recovery release
- Document LGPL compliance expectations for source and binary releases
- Status: complete

Current state:
- the project has completed Phase 4 for the Windows/Linux supported baseline (v1.6.2-revival1 release docs updated)
- actual binary packaging for Windows will require a runner with msbuild/cmake
- remaining work focuses on Phase 5 and continuous structural cleanup

## Phase 5: Optional Expansion
- ~~Investigate missing historical GUI source~~ -> **Status: Verified missing.**
- ~~Decide whether to recover, replace, or drop the legacy GUI~~ -> **Status: Recreate.** The legacy GUI acts as a wrapper but is visually outdated (2005). The new goal is to build a modern, cross-platform UI wrapper that controls the 2026 CLI.
- Revisit localization strategy beyond archived `.chm` help files
- Plan the modern GUI stack:
  - **Core Logic:** The robust `mp3gain` CLI (C99).
  - **Frontend:** Avalonia UI (C# / .NET). Chosen for its ability to flawlessly recreate the classic desktop "Grid" feel while rendering natively across Windows, Linux, and macOS via Skia, outputting high-performance standalone NativeAOT binaries.

## Immediate Next Slice
1. Continue structural cleanup of remaining legacy low-level state (frame-scan decoupling).
2. Keep macOS outside the release support claim until it is validated separately.
3. The corruption mismatch against `ffmpeg` has been evaluated and is now accepted as a permanent characteristic of `libmpg123` tolerance. No further C code changes will be made to emulate `ffmpeg` strictness.
