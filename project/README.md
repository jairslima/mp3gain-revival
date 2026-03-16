# Active Source Tree

This directory contains the extracted contents of the historical `mp3gain-1_6_2-src.zip` package.

## What Is Here
- Legacy C sources for the MP3Gain command-line tool
- Legacy Visual Studio solution and project files
- A simple Unix-oriented `Makefile`
- ReplayGain DLL wrapper sources

## Important Caveats
- The Visual Studio project references `mpglibDBL/*` files that are not present in this package
- The `Makefile` links against an external `libmpg123`
- Legacy resource files are historical; the CMake path generates a fresh Windows version resource for the CLI

## Immediate Maintenance Goal
Treat this tree as the base for reconstruction, not as a verified buildable release.

## Strategic Direction
- This source tree is being maintained as a derivative continuation of the original MP3Gain codebase.
- Modernization work should preserve original license notices while steadily reducing legacy complexity.
- The long-term target is a simpler portable core that can be built and tested consistently across major operating systems.

## Preferred Build Entry
- Use `CMakeLists.txt` in this directory for the active recovery path
- Treat `Makefile` and legacy Visual Studio files as historical build references

## Day-To-Day Workflow
- Limit code search and review to this directory unless a task explicitly needs repository-level docs or archival references
- Do not inspect `..\build\` or `..\vcpkg_installed\` as part of normal source analysis
