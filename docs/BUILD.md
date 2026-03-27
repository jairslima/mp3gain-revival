# Build Notes

The active build target for the revival effort is the command-line executable in `project/`.

## Current Build Strategy
- Prefer CMake over the legacy Visual Studio project files
- Treat the legacy `.sln`, `.vcproj`, `.dsp`, and `.dsw` files as historical references
- Use an external `libmpg123`
- Treat `build/` and `vcpkg_installed/` as local build/dependency state, not as source directories to inspect routinely

## Requirements
- CMake 3.16 or newer
- A C compiler
- `libmpg123` development headers and library

## Configure

```powershell
cmake -S project -B build
```

If `libmpg123` is installed in a non-standard location, pass it explicitly:

```powershell
cmake -S project -B build `
  -DMPG123_INCLUDE_DIR=C:\path\to\include `
  -DMPG123_LIBRARY=C:\path\to\lib\mpg123.lib
```

## Build

```powershell
cmake --build build --config Release
```

## Smoke Tests

- The CI smoke test script uses immutable fixtures from `test/fixtures/`
- Each run copies those fixtures to a temporary directory before applying gain changes or undo operations
- Do not depend on local working copies in `test/` for CI correctness
- The smoke script now keeps its temp files inside the repository tree so the same fixture-copy flow works both with the Linux binary and with `build/Release/mp3gain.exe` launched from `bash`/WSL
- When the smoke script selects the Windows executable, it converts MP3 file paths to Windows form before invoking the CLI so file arguments are not misread as `/` switches
- Current baseline: `27/27` smoke checks passing in the active workspace
- Current validated platforms:
  - Windows local build + CI
  - Linux local build + CI
  - macOS exploratory only, outside the immediate release support baseline
- Current smoke coverage includes:
  - version
  - track analysis
  - album analysis
  - apply track gain
  - undo
  - apply album gain
  - APE tag round-trip
  - ID3/APE mode separation
  - auto-clip flow
  - mixed valid/invalid batch handling via `test/fixtures/corrupt-truncated.mp3`
- A stable fixture that forces a true frame/decode failure path is still not locked in;
  several synthetic corruption candidates have been probed, but current coverage still
  reliably enforces missing-frame rejection and partial batch failure
- `test/probe_corruptions.ps1` exists as a deterministic probe harness for comparing
  corruption candidates while that fixture gap remains open
- The corruption probe now auto-detects an available Linux binary from `build/mp3gain` or `build-wsl/mp3gain` instead of depending on a single hardcoded path
- Overall validation snapshot is summarized in `docs/VALIDATION.md`

## Reproduction by Platform

### Windows

```powershell
cmake -S project -B build `
  -DMPG123_INCLUDE_DIR="vcpkg_installed/vcpkg/blds/mpg123/src/-0852196a3c.clean/src/include" `
  -DMPG123_LIBRARY="vcpkg_installed/vcpkg/blds/mpg123/x64-windows-rel/src/libmpg123/mpg123.lib"
cmake --build build --config Release
bash test/smoke_test.sh
```

### Linux

```bash
cmake -S project -B build-linux
cmake --build build-linux
bash test/smoke_test.sh
```

### macOS

- exploratory only until a validated local baseline exists

## Release Packaging Notes

### Windows
- Build with `cmake --build build --config Release`
- Ship `build/Release/mp3gain.exe` together with `build/Release/mpg123.dll`
- Keep `LICENSE` with binary redistributions and publish corresponding source

### Linux
- Build with `cmake --build build-linux` or another Linux-only build directory
- Ship the resulting `mp3gain` binary from that Linux build directory
- Document the target distro/toolchain used for the release build

## Known Issues
- The recovered Visual Studio project references missing `mpglibDBL/*` files
- The legacy `VerInfo.rc` is preserved for reference, but the CMake path now generates a fresh Windows version resource
- A broad recursive scan of the repository can be expensive because `vcpkg_installed/` contains a large dependency tree
- Legacy shared state is still concentrated in `project/mp3gain.c`, especially around low-level frame and write-path mutation
