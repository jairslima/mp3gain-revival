# Status Snapshot

## Current Phase

- Phase 4: final publication pending for the Windows/Linux baseline

## Validated Baseline

- `cmake --build build`: OK
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- Linux packaging: `pwsh -File scripts/package_release.ps1 -Platform linux -Version phase4-dev`
- Windows packaging: `pwsh -File scripts/package_release.ps1 -Platform windows -Version phase4-dev`

## Supported Release Baseline

- Windows
- Linux

macOS remains outside the first public release support claim until real validation exists there.

## Current Release Position

- Linux package generation is working.
- Windows package generation is working.
- Current local package artifacts:
  - `dist/mp3gain-linux-phase4-dev.zip`
  - `dist/mp3gain-windows-phase4-dev.zip`
- Final publication candidate version prepared in docs: `1.6.2-revival2`
- The MP3Gain/libmpg123 versus `ffmpeg` corruption mismatch is documented as a known limitation of the first public release.
- The remaining step for the CLI recovery baseline is public release publication, not core build/test recovery.

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

- Publish the first public release from a clean tagged snapshot and attach the validated Windows/Linux packages.
