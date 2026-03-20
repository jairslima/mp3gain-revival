# Status Snapshot

## Current Phase

- Phase 4: Release Readiness for the Windows/Linux baseline

## Validated Baseline

- `cmake --build build`: OK
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- Linux packaging: `pwsh -File scripts/package_release.ps1 -Platform linux -Version phase4-dev`

## Supported Release Baseline

- Windows
- Linux

macOS remains outside the first public release support claim until real validation exists there.

## Current Release Position

- Linux package workflow is working.
- Windows package workflow is documented but blocked in this workspace by missing `build/Release/mp3gain.exe` and `build/Release/mpg123.dll`.
- The MP3Gain/libmpg123 versus `ffmpeg` corruption mismatch is documented as a known limitation of the first public release.

## Key Files For The Next Developer

- `HANDOFF.md`
- `PROJECT.md`
- `ROADMAP.md`
- `docs/BUILD.md`
- `docs/VALIDATION.md`
- `docs/RELEASE.md`
- `docs/RELEASE_CANDIDATE.md`
- `CHANGELOG.md`
- `scripts/package_release.ps1`
- `test/probe_corruptions.ps1`
- `test/probe_corruptions.results.json`

## Immediate Next Step

- Produce a Windows Release build snapshot, then run `pwsh -File scripts/package_release.ps1 -Platform windows -Version <tag>`.
