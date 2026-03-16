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

## Current Risks
- Build has not been validated in this environment.
- The codebase may require dependency and project-file repair before first successful compile.
- Publishing as a current project is realistic for the CLI revival, not yet for the historical GUI.
- LGPL compliance should remain explicit in docs, notices, and release process as the code is modernized.
- `project\mp3gain.c` is still in legacy encoding, so edits there require extra care.
- The remaining highest-risk monolithic block is still the frame-by-frame analysis loop in `project\mp3gain.c`, but its decode/analyze and advance/progress portions have already been extracted.
- Current build blocker: `libmpg123` is still unavailable to the local CMake build.
- A `vcpkg install --triplet x64-windows` attempt was started from `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\vcpkg\`, but it either stalled or is progressing too slowly to confirm completion in-session.
- As of the last check, neither `vcpkg_installed\x64-windows\include\mpg123.h` nor the expected installed library files were present in a usable local path for this workspace.

## Recommended Next Steps
1. Initialize Git here if it is still not initialized outside this session.
2. Publish clear repository messaging that this is an LGPL fork/continuation of MP3Gain.
3. Pick the first supported target platform for build recovery.
4. Resolve `libmpg123` for the active toolchain, either by finishing the `vcpkg` install or wiring CMake to an already-installed copy.
5. Re-run CMake configuration and then compile to surface refactor-induced compile errors.
6. Fix compile errors before doing any more structural refactoring.
7. Add CI and minimum verification after the first successful build.

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
