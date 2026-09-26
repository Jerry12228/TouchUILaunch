# Developer and AI handbook

This handbook explains how to work on the standalone TouchUILaunch repository.
It serves Codex, Claude, other coding assistants, and human maintainers. The
project enables mobile UI for GI, SR, ZZZ, and WW; ZZZ additionally implements a
Windows touch bridge. Their implementations and validation requirements differ.

All command examples use PowerShell from the repository root: the directory
containing `CMakeLists.txt` and `AGENTS.md`. In a larger research workspace that
directory may be named `native/`. A standalone clone needs no parent workspace,
metadata dump, private research folder, or game binary for its default build
and tests. Paths such as `C:/Samples/...` are placeholders for separately
supplied inputs, not expected checkout contents.

## First reading

1. Read [AGENTS.md](../AGENTS.md) for the entry rules.
2. Read [AI working guide](AI_GUIDE.md) for the change and verification workflow.
3. Read [Architecture](ARCHITECTURE.md) for component and process boundaries.
4. Choose the chapters below for the task. There is no need to load the entire
   handbook into every AI session.

[CLAUDE.md](../CLAUDE.md) points to these same instructions. For an assistant
that does not discover either entry file, explicitly provide the reading prompt
in [AI working guide](AI_GUIDE.md#portable-reading-prompt). Do not assume every
tool automatically reads every linked document.

## Choose a task

| Task | Read next | Start tracing code here |
| --- | --- | --- |
| Set up, build, or package | [Development](DEVELOPMENT.md) | [Build script](../scripts/build.ps1), [CMake targets](../CMakeLists.txt) |
| Change CLI, logging, UAC, or launch behavior | [Game support](GAME_SUPPORT.md), [Testing](TESTING.md) | [Launcher](../launcher/src/launcher.cpp), [game registry and child ownership](../launcher/include/game.hpp) |
| Change SR / retained legacy UI initialization | [Game support](GAME_SUPPORT.md#gi-and-sr-initialization) | [Process runtime](../common/process/include/mobile_runtime.hpp), [GI/SR resolver](../games/gi/include/mobile_resolver.hpp) |
| Change the active GI 7.1 UI implementation | [Independent GI UI](GI_TOUCH_71.md) | [Initializer](../games/gi/include/gi_touch71.hpp), [patch plan](../games/gi/include/gi_touch71_plan.hpp) |
| Independently inspect the supplied GI 7.1 metadata | [Offline metadata recovery](GI_METADATA_71.md) | [Recovery tool](../games/gi/tools/recover_metadata71.py), [read-only query tool](../games/gi/tools/query_metadata71.py) |
| Change WW launch behavior | [Game support](GAME_SUPPORT.md#ww-launch) | [WW arguments](../games/ww/include/ww_launch.hpp) |
| Diagnose or extend ZZZ discovery | [ZZZ internals](ZZZ_INTERNALS.md#discovery-pipeline), [Testing](TESTING.md#external-sample-tests) | [Memory resolver](../games/zzz/include/profile_resolver.hpp), [semantic discovery](../games/zzz/include/short_discovery.hpp) |
| Change ZZZ touch or layout behavior | [ZZZ internals](ZZZ_INTERNALS.md#touch-state-and-frame-contract), [Testing](TESTING.md) | [Payload](../games/zzz/src/payload.cpp), [touch state](../games/zzz/include/touch_state.hpp) |
| Update generated schemas, rules, or evidence | [Development](DEVELOPMENT.md#generated-files), [ZZZ internals](ZZZ_INTERNALS.md#evidence-and-version-adaptation) | [ZZZ tools](../games/zzz/tools), [test data](../games/zzz/testdata/README.md) |
| Choose tests or report confidence | [Testing](TESTING.md) | [Verification script](../scripts/test.ps1), [CTest registration](../CMakeLists.txt) |
| Investigate a failure | [Troubleshooting](TROUBLESHOOTING.md) | Start with the reported stage and its owning module |
| Add another game | [Game support](GAME_SUPPORT.md#adding-or-changing-game-support), [Architecture](ARCHITECTURE.md) | [Registry](../launcher/include/game.hpp), an existing adapter, and its tests |

## What each document owns

- [AI working guide](AI_GUIDE.md): how to inspect, edit, verify, and hand off work.
- [Architecture](ARCHITECTURE.md): current structure, dependencies, process
  lifetime, and ownership boundaries.
- [Development](DEVELOPMENT.md): environment, build/package commands, generated
  files, source provenance, and coding conventions.
- [Game support](GAME_SUPPORT.md): CLI behavior and per-game implementations.
- [ZZZ internals](ZZZ_INTERNALS.md): resolver semantics, field meanings, input
  bridge, thread rules, and failure handling.
- [Testing](TESTING.md): checks, prerequisites, coverage, and acceptance evidence.
- [Troubleshooting](TROUBLESHOOTING.md): symptoms, diagnostics, and known tooling
  limitations.

End-user installation and examples remain in [English README](../README.md)
and [Chinese README](../README.zh-CN.md). Distribution notices remain in
[THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).

## Evidence and maintenance

Technical claims in this handbook link to checked-in code and tests. Follow
those links when changing behavior: the handbook is a guide to the source,
not a replacement for reading it. Comments, historical evidence, current
implementation, and desired design can disagree; record the difference before
deciding whether a task includes fixing it.

The versioned ZZZ files describe preserved evidence. They do not establish a
production version allowlist or prove that a current game installation works.
Keep build success, fixture success, sample validation, and real-game results
separate. See [Testing](TESTING.md#evidence-levels).

Update the chapter that owns a changed contract and keep links from the two AI
entry files working. Detailed synchronization rules are in
[AI working guide](AI_GUIDE.md#documentation-maintenance).
