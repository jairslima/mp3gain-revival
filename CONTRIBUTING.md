# Contributing

## Scope

- Active code lives in `project/`
- Active documentation lives in `docs/` and the repository root
- `archive/` is preserved historical material, not the active source tree

## Workflow

1. Prefer small, behavior-preserving refactors over large rewrites.
2. Preserve LGPL notices and existing copyright headers in recovered files.
3. Do not treat legacy Visual Studio files as the primary build path.
4. When touching behavior, validate with the smoke suite.

## Build

```powershell
cmake -S project -B build
cmake --build build --config Release
```

On Linux or WSL:

```bash
cmake -S project -B build
cmake --build build
```

## Tests

```bash
bash test/smoke_test.sh
```

- The smoke suite currently passes `27/27` checks in the active workspace.
- Fixtures in `test/fixtures/` are immutable inputs; the test script copies them to a temporary directory before mutating files.

## Refactor Priorities

- Keep moving orchestration out of `project/mp3gain.c`
- Reduce shared global state between `project/mp3gain.c` and `project/process.c`
- Prefer narrow helper extraction and explicit error propagation over `exit(...)` in recoverable paths

## Current Gaps

- A stable fixture that forces a true decoder/frame-corruption failure is still pending.
- The repository does not include the historical Windows GUI source.
