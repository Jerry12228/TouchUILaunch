# Repository Guidelines

## Start here

This directory is the standalone TouchUILaunch repository. Run commands from
this root, even when it is named `native/` inside a larger workspace.

Read [the AI working guide](docs/AI_GUIDE.md) and
[the architecture overview](docs/ARCHITECTURE.md), then use
[the documentation index](docs/README.md) to select task-specific chapters.
These documents are shared by all AI assistants; read the relevant chapters
before editing. Source, build configuration, and tests establish current
behavior. Report discrepancies instead of silently changing behavior to match
a stale description.

## Project boundaries

- `launcher/` owns CLI selection, logging, elevation, and launch coordination.
- `games/{gi,sr,ww,zzz}/` owns game behavior; ZZZ includes discovery and the DLL
  touch bridge. Keep game signatures out of the launcher.
- `common/` contains Windows, binary, process, and test helpers. Keep new shared
  utilities independent of game modules; existing process-layer coupling is
  documented in [Architecture](docs/ARCHITECTURE.md#current-dependencies-and-maintenance-direction).
- Launch only newly created games. Reject an already-running game and preserve
  owned-child cleanup. Do not introduce attachment to discovered PIDs.
- ZZZ runtime discovery reads the loaded module once. Keep file probing,
  fingerprints, and test address oracles outside the production resolver.
- Preserve callback lifetimes, compare/exchange ownership, window-thread UI
  calls, and consistent touch snapshots. See [ZZZ internals](docs/ZZZ_INTERNALS.md).

## Commands and generated files

Use Windows x64, Visual Studio 2022 with Desktop development with C++ and MASM
x64, and Python 3.10+ for verification. CMake 3.24 is the project minimum; the
checked-in presets require 3.25 or later.

```powershell
./scripts/build.ps1        # Configure, build Release, package into dist/
./scripts/test.ps1         # Verify existing build and package artifacts
./scripts/full.ps1         # Build, package, and verify
ctest --preset windows-x64-release
python games/zzz/tools/generate_profiles.py --check
```

Default tests require no game binaries. External sample configuration, focused
tests, and limitations are in [Testing](docs/TESTING.md). Report skipped checks;
passing fixtures does not establish real-game or streaming behavior.

Regenerate profile headers with `python games/zzz/tools/generate_profiles.py`.
Scan-rule regeneration additionally needs Capstone and an explicitly supplied,
preserved 3.1 sample. Follow [Development](docs/DEVELOPMENT.md#generated-files);
do not hand-edit generated headers. Keep game binaries outside Git and build
and package outputs under ignored `build/` and `dist/`.

## Style, tests, and contributions

Use C++20, four-space indentation in C++/Python/PowerShell and two spaces in
CMake. Match surrounding formatting; prefer `snake_case` filenames, functions,
and variables and `PascalCase` types. Headers use `.hpp` and `#pragma once`.
There is no checked-in formatter/linter configuration; MSVC uses `/W4`.

Tests live in module `tests/` directories and `common/process/mobile_tests.cpp`.
Name C++ tests `*_tests.cpp` and register targets/cases in `CMakeLists.txt`.
Add regression coverage for changed behavior; no coverage percentage is set.

Use Git as a careful human maintainer would: review the actual diff, stage only
task-related changes, and make focused, reviewable commits with meaningful
subjects and bodies. Follow the shared [Git workflow and commit-message rules](docs/AI_GUIDE.md#git-workflow-and-commit-messages),
including authorization for commits/pushes and protection of existing history.
No consistent Conventional Commits scheme is established. PRs describe affected games,
behavior changes, linked issues where applicable, and actual verification.
Include screenshots for visible UI changes. Update both user READMEs when
usage changes and the relevant handbook chapters when implementation contracts
change. Preserve third-party attribution and licenses.
