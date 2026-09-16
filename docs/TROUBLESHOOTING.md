# Troubleshooting

Start with the failing stage and the scope of the evidence. A build failure,
sample mismatch, launcher error, and asynchronously disabled touch bridge need
different investigations. Use [Testing](TESTING.md) to reproduce with owned
fixtures or read-only inputs before drawing conclusions about gameplay.

## Collect the right diagnostics

Normal operation and caught launcher errors are silent without `--log`.
Check the exit code. For an intentional game launch, use the relevant command
in [Game support](GAME_SUPPORT.md#launch-elevation-and-diagnostics) with logging.
For file-only ZZZ investigation, use its documented `--probe` command instead.

| Source | Location or marker | Interpretation |
| --- | --- | --- |
| Launcher | `logs/launcher-<launcher-pid>.log` beside the EXE | Selected path, stage transitions, `ERROR: stage=...`, relaunch result |
| Elevated launcher | A second launcher log with a different PID | Inspect this for work performed after UAC |
| ZZZ payload | `logs/touch-<game-pid>.log` beside the DLL | Resolver stages, fields, window/hooks, UI changes, recurring status, later failures |
| Boundary verifier | `memory-boundary.json` under the selected dist directory | Static source/import/fingerprint check scope and source digests |
| CTest | Output on failure; build tree's `Testing/Temporary/LastTest.log` after execution | Individual fixture/sample failure; enumeration alone produces no pass result |

[launcher_log.hpp](../launcher/include/launcher_log.hpp) keeps default logging
off and warns if it cannot create its file. The payload's
[log function](../games/zzz/src/payload.cpp) uses the retained logging event and
can silently fail to create a log directory/file. Missing logs alone do not
identify the underlying cause. Check the chosen artifacts, path permissions,
whether `--log` was supplied, and which process emitted the message.

## Build and verification setup

| Symptom | Check and next action |
| --- | --- |
| CMake rejects preset schema | The file uses schema 6; use CMake 3.25+. The project/script minimum of 3.24 does not make schema 6 readable by 3.24 |
| Visual Studio generator or MASM unavailable | Verify VS 2022 Desktop development with C++, Windows SDK, and x64 MASM; keep the `Visual Studio 17 2022`/x64 configuration |
| Platform error or pointer/layout mismatch | Check Windows x64 and Release configuration; do not disable ABI assertions to get a build |
| `Build directory does not exist` or missing packaged artifacts | Run `build.ps1` or `full.ps1` with the same build/dist paths; `test.ps1` does not compile or install |
| Tests pass but expected code change is absent | Rebuild and reinstall; built EXE and packaged EXE/DLL are different inputs to the suite |
| Unexpected files in `dist` | CMake installation does not clean old files; inspect install manifest and destination before packaging |
| `python` missing or wrong interpreter | Supply `-Python` to `test.ps1`/`full.ps1`; scan-rule generation additionally needs Capstone in that interpreter |
| No tests or incorrect subset | Inspect CTest enumeration in the actual configured tree, not just test executable filenames |

Commands and sources: [Development](DEVELOPMENT.md),
[CMake configuration](../CMakeLists.txt), [presets](../CMakePresets.json), and
[verification wrapper](../scripts/test.ps1).

## Samples, generators, and known tooling limitations

| Symptom | Explanation and supported next step |
| --- | --- |
| `-SRSample requires -ZZZSampleRoot` | Current wrapper registration groups external samples; supply the ZZZ root, or run the built `MobileUITests.exe <SR-file>` directly for a standalone read-only check |
| Missing ZZZ version folder | The wrapper requires all four directories, 2.5/2.6/3.1/3.2; use a direct single-sample discovery executable if only one preserved input is available |
| External tests use old paths | `test.ps1` does not configure CMake; inspect cached `TOUCHUI_*` values and reconfigure the selected tree |
| “SKIP” printed but CTest still attempts samples | The message follows script arguments, while registered cases follow the existing CMake cache; use separate trees and confirm registrations |
| An old SR sample case remains | Omitting an SR path does not clear a cached value; explicitly configure `TOUCHUI_SR_SAMPLE` to the intended value or empty string |
| Generated profile check names `scripts/generate_touch_profiles.py` | The stale-error text retains an obsolete path; the actual generator is `games/zzz/tools/generate_profiles.py` |
| `validate_profile.py` fails with undefined `ROOT` | Default path construction is broken; explicitly provide `--version` and `--game-dir` |
| Static validator says an older version is unknown | Its default candidates are 3.1/3.2; add the corresponding JSON via `--extra-profile` for 2.5/2.6 |
| New version JSON has no effect | Generator enumeration and CTest registration are explicit; review both alongside evidence changes |
| Scan-rule check fails or Capstone is missing | Verify the interpreter and exact preserved 3.1 input; inspect differences without overwriting the known evidence automatically |
| Discovery test cannot find an oracle | The supplied file hash is not in the test-only table; this is not proof that production discovery rejects unknown hashes |

See [generated-file commands](DEVELOPMENT.md#generated-files) and
[external-sample commands](TESTING.md#external-sample-tests). The sources for
the stale path and default-root defects are
[generate_profiles.py](../games/zzz/tools/generate_profiles.py) and
[validate_profile.py](../games/zzz/tools/validate_profile.py). These are current
limitations, not intended behavior. Keep the notes synchronized when fixed.

## Selection, UAC, and process ownership

For argument errors, check exactly one selector, the required explicit path,
the actual executable filename, and whether an option is supported. WW's
cloud flags are internally supplied game arguments, not extra launcher
options. Use `--log` to see validation errors.

If the launcher reports an already-running game, it matches the selected
family's executable names across installation paths. Exit that game normally
before starting another session. Changing `--game` to a second installation
does not bypass this contract. Do not “fix” the error by adding attachment,
wrapping a discovered PID in `game::Child`, or terminating an unrelated game.

“Another launcher is starting this game” comes from the per-game mutex. Check
whether another launcher invocation is active. Do not bypass serialization.

UAC cancellation is returned to the original launcher. A relaunch that still
lacks an elevated token fails its guard rather than asking repeatedly. The
internal `--elevation-relaunch` marker cannot grant privileges. For lost
arguments, review [quoting helpers](../common/windows/include/elevation.hpp)
and [elevation tests](../common/windows/tests/elevation_tests.cpp), including
spaces, Unicode, quotes, and trailing backslashes. Inspect both launcher logs.

If a newly created game exits after setup fails, consult
[Child ownership](../launcher/include/game.hpp): failure cleanup deliberately
terminates the still-owned child. After successful release, launcher exit
should not terminate it. This distinction is covered by
[process tests](../common/process/mobile_tests.cpp).

## GI, SR, and WW failures

| Stage/symptom | Investigate |
| --- | --- |
| Loader initialization/module load timeout | Child architecture, actual system API host mapping, module path, and the last stage; retained remote memory may still be in use until child cleanup |
| GI main image has no supported matches | Confirm main `il2cpp` versus legacy UserAssembly selection; use the correct read-only sample and inspect resolver output |
| GI conflicting decoded targets | Different matching sites disagree; retain rejection and inspect evidence rather than selecting the first match |
| GI initialization already hooked | Existing entry branch conflicts with installation; do not overwrite an unknown hook |
| GI hook installed but UI unchanged | Installation precedes target invocation; review object readiness and stub outcome/fixture coverage, then observe a controlled real session |
| SR zero/multiple matches | Review all three resolver variants and decoded writable state; do not choose an arbitrary candidate |
| SR worker installed but UI unchanged | Started flag proves worker activity, not visible UI; distinguish signature correctness from game behavior |
| WW starts without desired UI | Confirm exact cloud arguments and target executable; fixture success cannot prove this game build honors the flags |

The [game implementation chapter](GAME_SUPPORT.md#gi-and-sr-initialization)
links the adapters, resolver, shared process implementation, and MASM sources.
GI/SR input support must be assessed on its own terms; there is no ZZZ-style
Windows bridge installed for those games.

## ZZZ file probe and memory discovery

A probe failure can concern the executable path, GameAssembly mapping,
semantic discovery, or missing adjacent payload. Probe is a read-only file
check even when a real installation path is supplied. A successful probe
does not establish that the later loaded/hotfixed memory has identical code.

For runtime failures, find the last resolver stage in the payload log:

- **Retention/capture failure:** verify the expected loaded module, allocation
  ownership, readable committed pages, and PE/header validity. Guard,
  execute-only, inaccessible, or partial reads are rejected. Do not hide
  `MemoryReadError` as an ordinary failed signature candidate.
- **No unique wrapper or layout choice:** compare the input/code evidence
  with the matching rules and instruction consumers. A short anchor is not
  sufficient proof. Preserve bounds and ambiguity rejection.
- **Property/dispatch mismatch:** trace both interface paths, ownership,
  argument preservation, and result-word roles. Compiler register/order
  changes may need a proven new variant, not a fixed-address fallback.
- **Invalid slot or function relationship:** distinguish file storage
  assumptions from initialized memory and reject aliased/out-of-bounds fields.

Use [ZZZ internals](ZZZ_INTERNALS.md#discovery-pipeline),
[memory tests](../games/zzz/tests/memory_tests.cpp), and
[sample discovery tests](../games/zzz/tests/discovery_tests.cpp). Do not turn a
test oracle into a production version table to make a failing sample pass.

## ZZZ readiness and missing input

`Payload started` occurs before discovery, and `READY` occurs before successful
UI/input acceptance. If the launcher already returned zero, inspect the
payload log for later errors. For a GameAssembly, window, or icall timeout,
use the reported stage to distinguish missing modules, window selection, and
callback readiness. Avoid blind reinjection after a loader timeout.

The periodic status line exposes these diagnostic hints:

| Fields | Interpretation |
| --- | --- |
| `enabled`, `UI` | Current bridge enable state and last observed effective layout; `UI=-1` means no successful observation yet |
| `WM_TOUCH`, `WM_POINTER` | Messages seen by the subclass; counts alone do not prove accepted touch contacts |
| `downs`, `dropped` | Accepted state downs and downs rejected at the contact capacity bound |
| `count_calls` | Calls through the hooked count callback |
| `bridge_frames`, `bridge_count` | Frames selecting bridge snapshots and last bridge contact count |
| `native_count_last_fallback` | Last count from a native fallback call; it may be stale while bridge input is active |

If message counts stay zero during intended touch, investigate the selected
window, focus, registration, and how the device/streaming client delivers
input. If messages arrive without contacts, inspect enable state, `PT_TOUCH`
filtering, event-source selection, and down/up order. If contacts appear but
the UI does not react, examine callback use, frame counts/dimensions, UI
provider readiness, and the visible game state. These are diagnostic leads,
not automatic root-cause conclusions.

For misaligned input, follow screen-to-client normalization, Unity dimensions,
and Y inversion. For stuck/duplicate contacts, inspect source deduplication,
cancellation, terminal-frame retirement, stable finger IDs, and the two-frame
native echo suppression. Reproduce the logic in
[TouchState](../games/zzz/tests/touch_state_tests.cpp) and the
[bridge harness](../games/zzz/tests/bridge_test_body.inc) before changing it.

## Disabled bridge and recovery

A UI native-memory fault disables further UI calls for that process. A hook
slot changed by another writer disables input and attempts conditional
restoration of still-owned slots. Destruction of the selected game window also
disables the bridge; it does not automatically select a replacement window.

Preserve the relevant logs and restart the game before a fresh launch.
There is no supported live DLL unload, public `--disable`, or reattachment
command. Do not free resident callbacks or overwrite competing pointers as a
recovery shortcut. Internal rollback and ownership behavior are described in
[ZZZ internals](ZZZ_INTERNALS.md#hook-installation-and-failure-handling).

When handing off an unresolved issue, include the selected game/build, exact
command, artifact location, last stage, pertinent error/status lines, and
which fixture/sample/manual checks ran. Avoid embedding large game binaries
or local private paths in repository documentation.
