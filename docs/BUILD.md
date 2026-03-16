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

## Known Issues
- The recovered Visual Studio project references missing `mpglibDBL/*` files
- The legacy `VerInfo.rc` is preserved for reference, but the CMake path now generates a fresh Windows version resource
- This workspace has not yet completed a real compile validation
- A broad recursive scan of the repository can be expensive because `vcpkg_installed/` contains a large dependency tree
