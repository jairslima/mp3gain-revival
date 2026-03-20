# Validation Status

## Current Baseline

- Smoke suite: `27/27` passing
- Windows: build + smoke validated
- Linux: build + smoke validated
- macOS: exploratory, not part of the current release support baseline

## Platform Status

| Platform | Build | Smoke | Notes |
| --- | --- | --- | --- |
| Windows | validated | validated | local MSVC + `mpg123.dll` runtime |
| Linux | validated | validated | WSL2/Ubuntu-based validation path |
| macOS | exploratory | exploratory | no validated baseline; outside the immediate release support claim |

## CI Coverage

| CI job | Platform | Build | Smoke | Artifact |
| --- | --- | --- | --- | --- |
| `build-windows` | Windows / MSVC | yes | yes | `mp3gain-windows-x64` |
| `build-linux` | Linux / GCC | yes | yes | `mp3gain-linux-x64` |

macOS does not yet have an active CI job because it is outside the immediate release support baseline until a validated local path exists.

## Local Reproduction

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
cmake -S project -B build
cmake --build build
bash test/smoke_test.sh
```

### macOS

- exploratory only: no validated local command path has been locked in yet

## Phase 3 Gate

Phase 3 should be treated as effectively complete only when all of the following are true:

1. partial-failure batch behavior is stable and no longer depends on fragile process-wide abort paths
2. the corruption/decode investigation has a reproducible oracle-backed baseline beyond missing-frame rejection
3. the supported-platform story is explicit for release consumers
4. release packaging docs are aligned with the verification baseline

## Release Handoff Readiness

| Item | State | Note |
| --- | --- | --- |
| release checklist | ready | documented in `docs/RELEASE.md` |
| Windows package shape | ready, pending local artifacts | binary + DLL + license |
| Linux package shape | executed in workspace | binary + license + toolchain notes |
| supported-platform statement | ready | Windows/Linux validated; macOS outside current support baseline |
| Phase 4 gate | ready | explicit checklist in `docs/RELEASE.md` |
| verification blockers cleared | ready for Phase 4 | Windows/Linux baseline accepted; decoder mismatch documented as limitation |

## Current Gaps

- the deepest legacy shared state is still concentrated in `project/mp3gain.c`
- macOS has not been validated and is currently outside the release support claim
- the current MP3Gain/libmpg123 path still tolerates several corruption cases that an external decoder oracle flags as invalid

## Accepted Release Limitations

- macOS is outside the first public release support baseline
- the MP3Gain/libmpg123 path may tolerate some corruption classes that the `ffmpeg` oracle rejects

## Corruption Probe Snapshot

- `corrupt-truncated`: fails with non-zero exit and missing-frame rejection
- `payload-corrupt`: still analyzes successfully
- `scattered-zero`: still analyzes successfully
- `header-window`: still analyzes successfully
- `sync-byte`: still analyzes successfully
- `crc-sideinfo`: still analyzes successfully
- `transition-gap`: still analyzes successfully
- `main-data-begin`: still analyzes successfully
- `reservoir-burst`: still analyzes successfully
- `cross-frame-stitch`: still analyzes successfully
- `sideinfo-shear`: still analyzes successfully
- `late-payload-scramble`: still analyzes successfully
- `late-header-poison`: still analyzes successfully
- `frame-cluster-poison`: still analyzes successfully
- multiple stable candidates now also register decoder errors in the `ffmpeg` oracle path
- persisted results matrix: `test/probe_corruptions.results.json`

## Promotion Decision

- `corrupt-truncated.mp3` remains the smoke/CI fixture for missing-frame rejection and partial-batch failure.
- The rest of the stable corruption baseline is promoted into the oracle-backed probe matrix instead of the smoke suite, because the current MP3Gain/libmpg123 path still tolerates those inputs.

## Supporting Artifacts

- Build notes: `docs/BUILD.md`
- Release notes/checklist: `docs/RELEASE.md`
- Contributor workflow: `CONTRIBUTING.md`
- Corruption probe harness: `test/probe_corruptions.ps1`
