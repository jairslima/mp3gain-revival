# First Public Recovery Release Draft

## Scope

This release candidate targets the currently supported baseline:

- Windows
- Linux

macOS is not part of the first public recovery release claim.

## Verification Baseline

- build validated on Windows and Linux
- smoke suite baseline: `27/27`
- corruption probe baseline persisted in `test/probe_corruptions.results.json`

## Included Release Decisions

- partial-batch failure handling is considered stable enough for release
- `corrupt-truncated.mp3` remains the smoke/CI corruption fixture
- the difference between MP3Gain/libmpg123 and the `ffmpeg` corruption oracle is treated as a known limitation for the first release, not a release blocker

## Packaging Status

- Linux package: generated successfully in this workspace as `dist/mp3gain-linux-phase4-dev.zip`
- Windows package: requires a Windows Release build artifact and `mpg123.dll` in `build/Release/`
- Current workspace note: the local PowerShell environment does not expose `cmake`/MSBuild directly, so Windows packaging could not be completed from this snapshot
- Latest execution result: `pwsh -File scripts/package_release.ps1 -Platform windows -Version phase4-dev` fails with `Missing Windows executable: build/Release/mp3gain.exe`

## Release Notes Draft

- revives the MP3Gain CLI build on a modern CMake path
- validates Windows and Linux as the first supported release baseline
- adds fixture-based smoke coverage for analysis, apply, undo, tags, clipping, and mixed valid/invalid batches
- improves per-file failure handling so bad inputs no longer abort the whole active batch path
- documents corruption probing with a deterministic oracle-backed baseline

## Known Limitations

- macOS is not yet in the supported release baseline
- legacy low-level shared state still exists in `project/mp3gain.c`
- some corrupted MP3s that `ffmpeg` flags as decode-invalid are still tolerated by the current MP3Gain/libmpg123 path
