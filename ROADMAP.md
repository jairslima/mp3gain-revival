# Roadmap

## Phase 1: Repository Recovery
- Preserve the recovered historical material
- Establish a clean active source tree
- Add repository-level documentation
- Define a modern build entry point
- Establish clear workspace hygiene so source inspection stays scoped to active code and docs
- Document the project explicitly as an LGPL fork/continuation of MP3Gain

## Phase 2: CLI Build Recovery
- Validate the CMake-based build on one supported platform
- Confirm the required `libmpg123` dependency path
- Resolve compiler and portability issues in the active CLI sources
- Document exact build commands for Windows and Unix-like systems
- Remove or isolate legacy build assumptions that block portability
- Start separating `mp3gain.c` into CLI, engine, decoder, and tag-handling responsibilities

Current Phase 2 progress:
- CLI parsing has been moved out of `mp3gain.c`
- non-loop file preparation and simple execution branches have been moved out of `mp3gain.c`
- decoder setup and result/apply helpers have been moved out of `mp3gain.c`
- the main remaining extraction target is the frame-by-frame analysis loop

## Phase 3: Verification
- Add sample-based smoke tests for analysis mode
- Verify tag read/write behavior on representative MP3 files
- Check undo behavior and clipping-related flows
- Add continuous integration
- Stand up cross-platform validation for Windows, Linux, and macOS
- Introduce regression coverage before behavior-changing simplifications

## Phase 4: Release Readiness
- Write changelog and release notes for the revival branch
- Define supported platforms and toolchains
- Add packaging instructions
- Publish the first public recovery release
- Document LGPL compliance expectations for source and binary releases

## Phase 5: Optional Expansion
- Investigate missing historical GUI source
- Decide whether to recover, replace, or drop the legacy GUI
- Revisit localization strategy beyond archived `.chm` help files
- Consider whether a future GUI should be a separate frontend over the same portable core
