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

## Recommended Next Steps
1. Test actual MP3 analysis: `mp3gain.exe somefile.mp3`
2. Test gain application: `mp3gain.exe -r somefile.mp3`
3. Add smoke tests for analysis-only flows.
4. Set up GitHub repository and push.
5. Add CI (GitHub Actions) for Windows build validation.
6. Validate Linux build via WSL or CI.

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
