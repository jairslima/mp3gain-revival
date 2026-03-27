# Release Publish Plan

This document defines what is still missing to close the CLI recovery effort as a public release.

## Objective

Publish the first public MP3Gain Revival CLI release for the validated baseline:

- Windows
- Linux

macOS remains outside the release support claim for this first release.

## What Is Already Done

- CLI build recovery completed
- Windows and Linux smoke baseline validated
- `bash test/smoke_test.sh`: `27/27 passed`
- `pwsh -File test\probe_corruptions.ps1`: OK
- Linux package generated:
  - `dist/mp3gain-linux-phase4-dev.zip`
- Windows package generated:
  - `dist/mp3gain-windows-phase4-dev.zip`
- Release notes, validation docs, and changelog exist

## What Still Blocks Final Completion

Only publication work remains for the CLI recovery scope:

1. Choose the final public version/tag
2. Produce a clean release snapshot from that exact commit/tag
3. Attach the validated Windows/Linux packages to the public release entry
4. Publish the release notes with the supported-platform and limitation statements already documented

## Recommended Final Versioning

Current release candidate for the prepared publication snapshot is:

- `1.6.2-revival2`

This snapshot already includes release-facing fixes after the earlier `revival1` draft state, so `revival2` is the correct publication candidate unless more changes are introduced before tagging.

## Required Release Inputs

- [CHANGELOG.md](C:\Users\jairs\Claude\MP3Gain\CHANGELOG.md)
- [docs/RELEASE.md](C:\Users\jairs\Claude\MP3Gain\docs\RELEASE.md)
- [docs/VALIDATION.md](C:\Users\jairs\Claude\MP3Gain\docs\VALIDATION.md)
- [STATUS.md](C:\Users\jairs\Claude\MP3Gain\STATUS.md)
- [test/probe_corruptions.results.json](C:\Users\jairs\Claude\MP3Gain\test\probe_corruptions.results.json)

## Final Publication Checklist

- [ ] Confirm working tree is clean
- [ ] Confirm intended release commit hash
- [ ] Confirm final version/tag name
- [ ] Verify `dist/mp3gain-linux-<version>.zip`
- [ ] Verify `dist/mp3gain-windows-<version>.zip`
- [ ] Confirm Windows zip contains:
  - `mp3gain.exe`
  - `mpg123.dll`
  - `LICENSE`
  - `CHANGELOG.md`
- [ ] Confirm Linux zip contains:
  - `mp3gain`
  - `LICENSE`
  - `CHANGELOG.md` if intentionally included
- [ ] Create/push the git tag
- [ ] Publish GitHub release notes
- [ ] Attach both zips to the release

## Explicit Non-Blockers

These do not block the first CLI release:

- further refactoring of `project/mp3gain.c`
- macOS validation
- Avalonia GUI completion
- changing `libmpg123` behavior to emulate `ffmpeg` strictness

## Post-Release Tracks

After the CLI release is public, the next tracks are:

1. Continue structural cleanup in the legacy C core
2. Decide whether macOS enters the supported baseline
3. Continue the Avalonia GUI as a separate product track
