# MP3Gain Revival

This repository is being rebuilt from archived MP3Gain materials with the goal of turning the command-line core into a maintainable public project again.

## Project Position
- This effort is intended to continue MP3Gain as a legitimate fork of the abandoned original project.
- The recovered source files and bundled license text indicate that the code was distributed under the GNU LGPL 2.1 or, at the user's option, any later version.
- The practical consequence is that this repository can modernize, modify, and redistribute the original codebase as long as the LGPL obligations are preserved.
- This repository should therefore present itself as a fork and derivative continuation, not as an unrelated clean-room reimplementation.

## License Direction
- Preserve the original copyright and license notices in recovered files.
- Keep the LGPL license text with the project and with redistributions.
- Mark modified files clearly when modernizing or refactoring recovered code.
- Distribute corresponding source code for released binaries derived from this codebase.
- Treat this repository's legal posture as "fork under LGPL", while recognizing that formal legal advice is outside the scope of repository documentation.

## Product Direction
- Modernize the legacy CLI into a smaller, maintainable, cross-platform tool.
- Reduce platform-specific legacy code and obsolete project files over time.
- Prefer a portable core with a thin CLI entry point.
- Current supported release baseline: Windows and Linux.
- Revisit macOS support only after a validated build-and-smoke baseline exists there.

## Working Scope
- Active code lives in `project/`
- Active documentation lives in `docs/` and the repository root
- `archive/` is preserved historical material and should not be treated as the active source tree
- `build/` and `vcpkg_installed/` are generated or dependency directories and should be ignored during normal code inspection

## Search Hygiene
- Prefer searches rooted in `project/` or `docs/`
- Avoid recursive scans across the full repository unless the task explicitly requires archival or dependency inspection
- `rg` should ignore `build/`, `vcpkg_installed/`, `archive/`, and other generated directories via `.rgignore`

## Repository Layout
- `project/`: extracted `mp3gain-1_6_2-src.zip` source tree, treated as the active codebase
- `archive/source/`: original source archive as recovered
- `archive/binaries/`: historical binary packages
- `archive/translations/`: historical translated help packages
- `archive/site/`: mirrored pages and assets from the original project site and related references

## Current Scope
- Recover and document the legacy CLI implementation
- Make the source tree understandable and versionable
- Prepare the project for modern build and CI work
- Define a compliant and explicit path to continue the original project as a maintained fork
- Simplify the codebase enough to support long-term cross-platform maintenance

## Build Path
- Active build target: the CLI in `project/`
- Preferred build system: CMake
- Build notes: `docs/BUILD.md`
- Validation status: `docs/VALIDATION.md`
- Refactoring direction: `docs/ARCHITECTURE.md`

## Validation Snapshot
- Windows: build + smoke validated
- Linux: build + smoke validated
- macOS: exploratory, not in the immediate release support baseline
- Current smoke baseline: `27/27`
- Current corruption probe baseline: only the truncated missing-frame candidate fails; the synthetic decode-corruption candidates still analyze successfully

## Planning
- Recovery plan: `ROADMAP.md`
- Historical material guide: `archive/README.md`
- Contributor guide: `CONTRIBUTING.md`

## Known Gaps
- No Git history was recovered
- The Visual Studio project references `mpglibDBL/*` files that are not present in the extracted source package
- The Unix `Makefile` expects an external `libmpg123`
- Windows build validation has been completed locally, but CI stabilization and cross-platform validation are still incomplete
- The historical Windows GUI source is not present in this archive set
- The local dependency tree in `vcpkg_installed/` can become very large and is not part of the active source review path

## Recommended Next Steps
1. Execute Phase 4 release-readiness work for the Windows/Linux baseline
2. Prepare packaging and release notes for the first public recovery release
3. Keep the published `archive/` tree curated and clearly separated from the active source tree
4. Continue reducing global-state coupling where it materially improves release confidence
5. Revisit macOS only after a validated baseline exists there

## Release Baseline

- first public recovery release target: Windows and Linux
- macOS: explicitly outside the first release support claim
- known limitation: the current MP3Gain/libmpg123 path may tolerate some corruption classes that the `ffmpeg` oracle rejects

## License
The recovered source package includes the LGPL text. A copy is available at the repository root in `LICENSE`.

This repository is documenting the codebase as a derivative continuation of MP3Gain under the LGPL terms present in the recovered source. That statement is a technical reading of the project materials in this repository, not formal legal advice.
