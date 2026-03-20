These files are immutable smoke-test fixtures for CI and local validation.

The smoke test copies them to a temporary directory before running commands that
modify MP3 data (`-r`, `-u`, `-a`).

Included fixtures:
- `test1.mp3` and `test2.mp3`: valid reference files for analysis/apply/undo/tag flows
- `corrupt-truncated.mp3`: real corrupted MP3 fixture created by truncating `test1.mp3`
  to exercise partial-failure batch handling without generating test data on the fly

Current limitation:
- a stable fixture that reliably reaches the decoder/frame-corruption failure path
  is still pending; the current corrupted fixture covers missing-frame rejection and
  partial batch failure, which is already enforced in `test/smoke_test.sh`
- several synthetic payload/header corruption candidates have been probed during
  refactoring, but they currently remain too tolerant or too unstable to lock into CI

Reproduction aid:
- `test/probe_corruptions.ps1` runs a small deterministic probe set against the Linux
  test binary and an `ffmpeg` decoder oracle to compare:
  - frame discovery corruption
  - payload corruption with preserved frame headers
  - scattered corruption across the file
  - header-window corruption
  - sync-byte corruption across later frames
  - CRC/side-info corruption windows
  - transition-gap corruption between nominally valid frames
  - main-data-begin/early side-info corruption
  - reservoir-burst corruption inside later frames
  - cross-frame-stitch corruption straddling frame boundaries
  - sideinfo-shear corruption after stable sync
  - late-payload-scramble corruption after stable frame discovery
  - late-header-poison corruption across consecutive later frame headers
  - frame-cluster-poison corruption across a contiguous cluster of later frames
- current observed behavior from that probe set:
  - truncated corruption exits non-zero and reports missing valid MP3 frames
  - multiple stable corruption candidates now produce real decode errors in `ffmpeg`
  - the current MP3Gain/libmpg123 path still analyzes many of those same candidates successfully

Do not point the smoke test at mutable working files in `test/`.
