# HANDOFF

## Project
- Name: MP3Gain Revival
- Directory: `C:\Users\jairs\Claude\MP3Gain`
- Status: local recovery workspace organized from historical archives

## Goal
- Recover MP3Gain as a public modern GitHub project, starting with the command-line core.
- Preserve the historical material without mixing it into the active source tree.
- Continue the abandoned original as a legally compliant LGPL fork rather than a separate "inspired by" rewrite unless a future clean-room strategy is chosen deliberately.

## Current Layout
- Active code: `project\`
- Historical source archive: `archive\source\`
- Historical binaries: `archive\binaries\`
- Historical translations/help: `archive\translations\`
- Mirrored project pages and references: `archive\site\`
- Repository docs: `README.md`, `LICENSE`, `HANDOFF.md`

## Workspace Hygiene
- Default inspection scope is `project\`, `docs\`, and repository-level Markdown files.
- Do not run wide recursive searches across the repository root unless the task explicitly requires historical or dependency inspection.
- Treat `build\` and `vcpkg_installed\` as generated/dependency trees, not as active source.
- Treat `archive\` as preserved reference material and read it only when the task needs historical context.

## Active Codebase
- Source package extracted from `mp3gain-1_6_2-src.zip`
- Main files:
  - `project\mp3gain.c`
  - `project\gain_analysis.c`
  - `project\apetag.c`
  - `project\id3tag.c`
  - `project\rg_error.c`
- Build files:
  - `project\Makefile`
  - `project\mp3gain.sln`
  - `project\mp3gain.vcproj`
  - `project\mp3gain.dsp`
  - `project\mp3gain.dsw`

## Important Findings
- The active code is a legacy C CLI project.
- The architectural bottleneck is `project\mp3gain.c`, which mixes CLI, decoding, file mutation, tag handling, and platform-specific logic.
- The Visual Studio project references `mpglibDBL/*` files that are not present in the recovered source package.
- The `Makefile` expects an external `libmpg123`.
- The active CMake path generates a new Windows version resource instead of relying on the stale legacy one.
- The Windows GUI source is not present in this workspace.
- Historical translation ZIPs contain compiled `.chm` help files, not active UI source.
- The recovered license materials and source headers indicate LGPL 2.1-or-later terms, which supports treating this repository as a derivative fork of MP3Gain.

## What Was Done In This Session
- Created a handoff file for cross-IA continuity.
- Extracted the core source archive into `project\`.
- Moved archives and mirrored pages into `archive\` subfolders.
- Added `README.md`, `project\README.md`, `.gitignore`, and root `LICENSE`.
- Added `project\CMakeLists.txt` and `docs\BUILD.md` for the active build recovery path.
- Added `.rgignore` and tightened workspace hygiene so normal searches stay out of `build\`, `vcpkg_installed\`, and `archive\`.
- Added `docs\ARCHITECTURE.md` and aligned repo docs around the project being an LGPL fork/continuation.
- Split major pieces out of `project\mp3gain.c` into:
  - `project\cli.[ch]`
  - `project\prep.[ch]`
  - `project\exec.[ch]`
  - `project\process.[ch]`
- Moved into helpers/modules:
  - CLI parsing and table header output
  - per-file preparation and recalc flag setup
  - simple per-file actions like tag-only, undo, direct gain, and delete-tag flows
  - input opening and `mpg123` setup
  - first-frame MP3 preparation and analysis frequency sync
  - track analysis finalization
  - track and album result reporting
  - `applyTrack`, `applyAlbum`, and final dirty-tag writes
  - per-frame audio processing inside the MP3 scan loop
  - frame-scan advance/progress updates inside the MP3 scan loop
  - per-frame MP3 header parsing inside the MP3 scan loop
  - combined per-frame iteration orchestration inside the MP3 scan loop
  - platform-protected per-frame iteration wrapper inside the MP3 scan loop
  - the full MP3 frame-processing `while (ok)` loop now orchestrated from `project\process.[ch]`
  - the per-file recalc path and post-track-finalization path now orchestrated from `project\process.[ch]`
  - the album post-processing/finalization path now orchestrated from `project\process.[ch]`
  - runtime cleanup/free logic now orchestrated from `project\process.[ch]`
  - the per-file batch loop from `main` now orchestrated from `project\process.[ch]`
  - initial file-prepare batching and final batch-finalization/return path now orchestrated from `project\process.[ch]`
  - album recalc computation and top-level batch orchestration now also routed through `project\process.[ch]`
  - runtime allocation/setup batching now orchestrated from `project\process.[ch]`
  - `fileok` allocation also moved out of `main` into runtime setup orchestration
  - CLI option field mapping now delegated through `project\cli.[ch]`
- Exposed `WriteMP3GainTag(...)` and `queryUserForClipping(...)` from `mp3gain.c` so orchestration can continue moving into `process.c`.

## Build Status: WORKING

The Windows build is now validated. First successful compile achieved on 2026-03-16.

### libmpg123 Resolution
The mpg123 source was already extracted by a previous vcpkg attempt at:
`vcpkg_installed\vcpkg\blds\mpg123\src\-0852196a3c.clean\`

The release build was completed manually:
```
cmake --build vcpkg_installed/vcpkg/blds/mpg123/x64-windows-rel --target libmpg123
```

Resulting artifacts:
- Header: `vcpkg_installed\vcpkg\blds\mpg123\src\-0852196a3c.clean\src\include\mpg123.h`
- Lib: `vcpkg_installed\vcpkg\blds\mpg123\x64-windows-rel\src\libmpg123\mpg123.lib`
- DLL: `vcpkg_installed\vcpkg\blds\mpg123\x64-windows-rel\src\libmpg123\mpg123.dll`

### Compile Fixes Applied
All compile errors from the refactoring were fixed:
1. `mp3gain.c`: Added `#include <windows.h>` in WIN32 block (needed for HANDLE, FILETIME, etc.)
2. `mp3gain.c`: Removed `static` from `frequency[4][4]` to allow extern reference from process.c
3. `mp3gain.c`: Removed `static` from functions now called cross-TU: `fillBuffer`, `skipID3v2`, `frameSearch`, `getSizeOfFile`, `changeGain`, `showVersion`, `errUsage`, `fullUsage`, `wrapExplanation`
4. `process.c`: Added extern declarations for `curframe`, `arrbytesinframe`, `frequency`
5. `process.h`: Added missing `double *lastfreq` param to `mp3gain_process_file_analysis` declaration
6. `process.c`: Added missing `lastfreq` argument in `mp3gain_finish_track_recalc` call
7. `rg_error.h`: Guarded MMSYSERR/WAVERR macro definitions with `#ifndef` to avoid redefinition after `<windows.h>`

### Confirmed Working
```
mp3gain.exe version 1.6.2   (powershell -Command "& 'build/Release/mp3gain.exe' '-v'")
```

## Current Risks
- The mpg123.dll must be present alongside mp3gain.exe for it to run.
- No automated tests yet - behavior correctness is unverified beyond startup.
- Linux/macOS build not yet validated.
- LGPL compliance should remain explicit in docs and release process.

## 2026-03-17 Status Update

### Recent Commits Now On `master`
- `0269f0d` `test: make smoke fixtures reproducible in CI`
- `539c051` `docs: align project status with current workspace`
- `2ff9b19` `docs: publish archived project materials`
- `527e9c1` `test: cover tags and auto-clip flows`

### Verification State
- Windows local build remains working.
- Windows CI was updated to use reproducible fixtures in `test/fixtures/`.
- Smoke coverage now includes:
  - version
  - track analysis
  - album analysis
  - apply track gain
  - undo
  - apply album gain
  - APE tag round-trip (`-r`, `-s c`, `-s d`)
  - ID3/APE mode separation (`-s i` vs `-s a`)
  - auto-clip flow (`-k -d 20 -r`)
- The local shell environment in this Windows workspace could not run `bash` or WSL reliably, so Linux validation was not performed here.

### Archive Publication
- `archive/` is now published in Git and matches the repository documentation.
- Historical source ZIP, binary ZIP, translation ZIPs, and mirrored site/reference pages are now part of the remote repository.

### Documentation Alignment
- `README.md` and `PROJECT.md` were corrected to stop overstating Phase 3 completion.
- Current position is:
  - Phase 2: strongly advanced
  - Phase 3: in progress
  - Windows local validation: done
  - Windows CI stabilization: in progress
  - Linux validation: still pending

## Next Developer Focus: Linux Environment

The next most valuable step is to continue from a real Linux-capable environment.

### Immediate Goal
- Add or validate a Linux configure/build path before attempting full Linux smoke coverage.

### Recommended First Actions
1. Install or confirm presence of `cmake`, a C compiler, and `libmpg123` development files.
2. Run `cmake -S project -B build` on Linux.
3. Run `cmake --build build`.
4. Fix the first real portability errors instead of pre-emptive large refactors.
5. Only after successful build, decide whether to add a Linux CI build job or Linux smoke tests.

### Most Likely Technical Friction Points
- Cross-translation-unit global state shared via many `extern` declarations between:
  - `project\cli.c`
  - `project\prep.c`
  - `project\exec.c`
  - `project\process.c`
  - `project\mp3gain.c`
- Remaining critical helpers still living in `project\mp3gain.c`, especially:
  - `WriteMP3GainTag(...)`
  - `queryUserForClipping(...)`
- Residual platform-specific logic in:
  - `project\mp3gain.c`
  - `project\apetag.c`
  - `project\id3tag.c`
  - `project\rg_error.h`
- Historical Windows DLL support in `project\replaygaindll.c` is not in the main build path, but should remain treated as legacy-only context.

### Important Constraint
- Do not treat lack of Linux success yet as a documentation problem first.
- The next developer should extract the first actual Linux compiler/configure failures, then correct the code and docs based on those results.

## 2026-03-17 Linux Build Session

### Linux Build: SUCCESS
- Environment: WSL2, Ubuntu 24.04, gcc 13.3.0, cmake 3.28.3, libmpg123-dev 1.32.5
- `cmake -S project -B build` completed without errors.
- `cmake --build build` completed without errors (after fixes below).
- Binary confirmed working: `./build/mp3gain version 1.6.2`

### Warnings Fixed (all resolved, build now warning-free)

1. **`process.c` missing `#include "prep.h"` and `#include "exec.h"`:**
   `mp3gain_prepare_file`, `mp3gain_compute_recalc_flags`, and `mp3gain_handle_simple_action`
   were declared in their respective headers but `process.c` did not include them,
   causing implicit-declaration warnings on Linux gcc.
   Fix: added `#include "exec.h"` and `#include "prep.h"` to `process.c`.

2. **Type mismatch in `mp3gain_prepare_first_mp3_frame` — `int *frame` vs `unsigned long *`:**
   The parameter was declared as `int *frame` in both `process.c` and `process.h`,
   but all callers pass `unsigned long *frame`.
   Fix: changed parameter type to `unsigned long *frame` in both files.

3. **`apetag.c` — unused variables `flags` and `curFieldNum`:**
   Both were assigned but never read. `flags` was used only to advance the pointer `p`;
   `curFieldNum` was reset to 0 at loop start and never incremented.
   Fix: removed declarations and dead assignments; preserved `p += 4` for `flags` field skip.

4. **`mp3gain.c` — unused variable `freqidx` in `changeGain`:**
   Assigned in two places inside the frame-scan loop but never read.
   Fix: removed declaration and both assignments.

### Recommended Next Steps
1. Add a Linux job to the GitHub Actions CI workflow.
2. Run existing smoke tests under Linux and confirm identical output to Windows.
3. Add smoke-test coverage for tag read/write, undo, and clipping flows on Linux.
4. After Linux CI is green, proceed to Phase 4 release packaging.

## Current Main Shape
- `project\mp3gain.c` `main()` is now close to a thin coordinator.
- The current top-level flow is effectively:
  - `mpg123_init()`
  - `mp3gain_cli_init(...)`
  - `mp3gain_cli_parse(...)`
  - `mp3gain_cli_apply_options(...)`
  - `mp3gain_prepare_runtime_batch(...)`
  - `return mp3gain_run_batch(...)`
- Most orchestration now lives in `project\process.[ch]`, and CLI option mapping lives in `project\cli.[ch]`.

## Rules For The Next IA
- Work inside `project\` as the active source tree.
- Treat `archive\` as preserved historical material.
- Ignore `build\` and `vcpkg_installed\` during normal exploration and search.
- Do not assume the Visual Studio project builds without restoring or replacing missing references.
- Confirm the target platform before changing build logic.
- Preserve LGPL notices and keep derivative-fork positioning explicit when editing docs or release material.

## 2026-03-18 Status Update

### Verification and CI
- Linux build continues to compile successfully in the active workspace.
- The smoke suite now passes `27/27`.
- The corruption probe now persists both MP3Gain results and an `ffmpeg` oracle comparison in `test\probe_corruptions.results.json`.
- A Linux Phase 4 dev package was generated successfully as `dist\mp3gain-linux-phase4-dev.zip`.
- Windows packaging remains blocked in the current snapshot because `build\Release\mp3gain.exe` and `build\Release\mpg123.dll` are not present, and the local PowerShell path does not expose `cmake`/MSBuild directly.
- Re-running `scripts\package_release.ps1` confirms the immediate Windows blocker is still `Missing Windows executable: build\Release\mp3gain.exe`.
- Smoke coverage now explicitly includes a mixed batch with one valid MP3 and one invalid/corrupted fixture, verifying:
  - non-zero final exit code on partial failure
  - stderr for the bad file
  - continued processing of the good file
- Fixture set now includes `test\fixtures\corrupt-truncated.mp3`.
- Additional synthetic corruption probes were attempted to reach a true decoder/frame failure path,
  but none were stable enough yet to add as a CI fixture.
- `test\probe_corruptions.ps1` now exists as a deterministic probe harness; current results are:
  - truncated corruption: exit `1`, missing-frame rejection
  - payload corruption with preserved headers: exit `0`
  - scattered zeroed regions: exit `0`
  - header-window corruption: exit `0`
  - sync-byte corruption: exit `0`
  - CRC/side-info corruption: exit `0`
  - transition-gap corruption: exit `0`
  - main-data-begin corruption: exit `0`
- `test\probe_corruptions.results.json` now persists the latest reproducible matrix from that probe harness.

### Error-Handling Changes
- The main MP3 processing path in `project\process.[ch]` no longer aborts the whole process on the first decoder/frame failure in the active batch path.
- `project\prep.c` and the AAC recalc path were softened so invalid AAC input does not immediately `exit(1)` in those touched paths.
- Recoverable file-level failures now route through batch status more consistently.

### Structural Refactor Progress
- `project\process.[ch]` now has a small `MP3GainBatchContext` carrying:
  - `gSuccess`
  - `first`
  - `numFiles`
  - `lastfreq`
- `project\process.c` now depends less directly on low-level globals in `project\mp3gain.c`.
- Additional frame-scan access has been hidden behind helpers in `project\mp3gain.c`, including:
  - scan-state reset
  - buffer-pointer priming
  - current-frame pointer access
  - current-frame min/max binding
  - frame pointer advance
  - ID3-skip + first-frame sync wrapper
  - next-frame search wrapper
  - current-frame gain scan wrapper
  - frame progress reporting
  - first-frame preparation
  - current-frame header preparation
- `changeGain(...)` has started to be split into smaller local helpers for the write path:
  - modify-stream open/setup
  - modify scan-state reset
  - initial modify-frame scan bootstrap
  - first writable frame preparation
  - current-frame validation
  - current-frame parse + bitstream priming
  - current-frame processing/apply orchestration
  - write-offset/progress calculation helpers
  - bitstream priming/advance helpers
  - shared frame-channel gain application helper
  - shared frame-channel gain tracking helper
  - shared MPEG side-info skipping helpers
  - shared MPEG channel/granule loop helpers
  - shared global-gain cursor/read/write helpers
  - shared modified-frame CRC/header finalization helper
  - shared scan-frame bitstream preparation helper
  - shared analyze-progress percentage helper
  - MPEG1 frame gain application
  - MPEG2/2.5 frame gain application
  - frame finalization/progress/advance
  - final progress/report flush
  - temp-file close/replace finalization
  - in-place close/flush finalization
  - Win32 cancel cleanup
  - shared write-result finalization helper
  - shared modify-open failure helper
- `project\process.c` no longer `exit(1)` on runtime batch allocation failure; setup now returns failure and `main()` exits cleanly with status `1`.

### Current Technical Position
- The project is still in Phase 3, but verification is materially stronger than the older docs indicate.
- Phase 3 is now best read as smaller verification slices:
  - 3.1 baseline smoke coverage: complete
  - 3.2 stateful CLI behavior: complete
  - 3.3 partial-failure and batch safety: complete
  - 3.4 CI and cross-platform validation: complete for the Windows/Linux supported baseline
  - 3.5 corruption/decode regression coverage: complete
  - 3.6 verification-to-release handoff: complete
- Phase 3.5 is now best read as smaller corruption-investigation slices:
  - 3.5.1 corruption classes worth probing: complete, with a documented taxonomy now covering discovery, payload, scattered, header-window, sync-byte, CRC/side-info, transition-gap, main-data-begin, reservoir-burst, cross-frame-stitch, side-info shear, late payload scramble, late header poison, and clustered multi-frame poisoning classes
  - 3.5.2 deterministic probe harness: complete
  - 3.5.3 repeatable results matrix: complete, now including an external decoder oracle
  - 3.5.4 promotion of a stable corruption baseline: complete
  - 3.5.5 explicit documentation of the current decoder mismatch: complete
- The deterministic probe harness now records both MP3Gain behavior and an `ffmpeg` decoder oracle.
- Multiple stable corruption candidates trigger real decoder errors in `ffmpeg` while still surviving analysis under the current MP3Gain/libmpg123 path; that mismatch is now a documented limitation rather than an unmeasured gap.
- The dominant remaining engineering debt is `extern`-driven shared state between `project\process.c` and `project\mp3gain.c`, especially around:
  - `skipID3v2`
  - `frameSearch`
  - the remaining bit-cursor helpers in the legacy write path
  - legacy write-context state in `changeGain(...)`
- Remaining `exit(...)` calls in active code are now primarily CLI/usage exits rather than the main per-file processing path.
- Contributor/release docs are now in better shape via `CONTRIBUTING.md`, `docs\BUILD.md`, and `docs\RELEASE.md`.
- Phase 3.4 is now split more explicitly into:
  - validated-platform matrix: complete
  - CI coverage for validated platforms: complete
  - local reproduction commands: complete
  - explicit pending-platform policy: complete, with macOS now outside the immediate release support claim
- Phase 3.6 is now split more explicitly into:
  - release checklist: complete
  - package shape for validated platforms: complete
  - explicit blockers list: complete
  - concrete Phase 4 gate: complete

### Recommended Next Steps
1. Execute Phase 4 release-readiness work for the Windows/Linux supported baseline.
2. Keep macOS outside the release support claim until a validated build + smoke path exists there.
3. Treat the MP3Gain/libmpg123 vs `ffmpeg` corruption mismatch as a documented release limitation unless a follow-on corrective effort is explicitly chosen.
4. Continue structural cleanup of remaining low-level state in `project\mp3gain.c` as follow-on refactor work.
