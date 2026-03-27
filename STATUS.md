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
- Windows release artifacts are present in this workspace:
  - `build/Release/mp3gain.exe`
  - `build/Release/mpg123.dll`
- Windows packaging was not re-run in this turn, but the previous "missing local artifacts" blocker no longer applies to this snapshot.
- The MP3Gain/libmpg123 versus `ffmpeg` corruption mismatch is documented as a known limitation of the first public release.

## Key Files For The Next Developer

- `HANDOFF.md`
- `PROJECT.md`
- `ROADMAP.md`
- `docs/BUILD.md`
- `docs/VALIDATION.md`
- `docs/RELEASE.md`
- `CHANGELOG.md`
- `scripts/package_release.ps1`
- `test/probe_corruptions.ps1`
- `test/probe_corruptions.results.json`
- `docs/AUDIT_2026-03-27.md`

## Immediate Next Step

- Run `pwsh -File scripts/package_release.ps1 -Platform windows -Version <tag>` against the current `build/Release` artifacts, then review the resulting package contents.
