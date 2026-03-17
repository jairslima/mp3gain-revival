These files are immutable smoke-test fixtures for CI and local validation.

The smoke test copies them to a temporary directory before running commands that
modify MP3 data (`-r`, `-u`, `-a`).

Do not point the smoke test at mutable working files in `test/`.
