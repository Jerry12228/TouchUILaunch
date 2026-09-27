# Testing and acceptance evidence

Run commands from the repository root in PowerShell. The primary entry point
is [test.ps1](../scripts/test.ps1), which verifies existing artifacts. Use
[full.ps1](../scripts/full.ps1) when they need building and packaging first.
See [Development](DEVELOPMENT.md) for toolchain requirements and directory
options.

## Evidence levels

| Evidence | What it supports | What it does not establish |
| --- | --- | --- |
| Source review / command enumeration | Current interfaces and registered checks | Successful compilation or test execution |
| Build and install | The selected configuration compiles/links and produces artifacts | Correct runtime or input behavior |
| Owned fixture tests | Logic, ABI, lifecycle, and rejection behavior exercised by those fixtures | Compatibility with a real game installation |
| External file / simulated loaded sample checks | Resolver agreement with a specific preserved input and test oracle | Injection, live hotfix state, physical input, or gameplay |
| Manual game session | Observed behavior for the recorded game build, hardware, and scenario | All versions, devices, or streaming clients |

Report each level separately. The default suite deliberately needs no game
binaries, and passing it is not a real-game compatibility claim. Sample version
names refer to recorded evidence, not a production allowlist.

## Default verification

From a fresh checkout with the required toolchain:

```powershell
./scripts/full.ps1
```

For a previously built and installed tree:

```powershell
./scripts/test.ps1
```

The verification script performs these steps:

1. Check the build directory, built launcher, and packaged EXE/DLL exist.
2. Run `generate_profiles.py --check` without modifying generated source.
3. Validate explicit external-sample options; external inputs are supplied by
   the caller, not discovered automatically in a parent workspace.
4. Run CTest against the selected build tree in Release configuration.
5. Compare GI signature discovery against the 7.1 IDA oracles if `-GiSample` was supplied.
6. Run Python CLI checks against the built launcher.
7. Check source boundaries and packaged imports/fingerprint strings, writing
   `memory-boundary.json` in the selected dist directory.

There are eight default CTest registrations in
[CMakeLists.txt](../CMakeLists.txt):

| CTest name | Executable | Main coverage |
| --- | --- | --- |
| `GITouch` | `GITouchTests` | Relocated GI discovery/fields, rejected conflicts, x64 UI/joystick execution and unchanged gesture deltas, protection, helper lifetime and owned suspended child cleanup |
| `MobileUI` | `MobileUITests` | Retained legacy GI/SR resolver variants, matching conflicts, assembly behavior on owned fixtures, suspended startup, remote loading, child ownership, extra-argument quoting, WW argument order/cwd |
| `TouchState` | `TouchStateTests` | Contact phases, multitouch, stable frame snapshots, Y conversion, deltas, recycled IDs, quick taps, ten-contact limit |
| `TouchBridge31` | `TouchBridgeTests 3.1` | Production bridge functions against owned window, callbacks, and 3.1 oracle layout |
| `TouchBridge32` | `TouchBridgeTests 3.2` | Same bridge harness with 3.2 oracle layout |
| `LauncherElevation` | `LauncherElevationTests` | Argument quoting, relaunch guard, cancellation/error propagation, owned argument round-trip child |
| `LauncherLog` | `LauncherLogTests` | Opt-in logs, default silence, Unicode paths, captured streams, restoration, error-dialog policy |
| `MemoryReader` | `MemoryReaderTests` | Raw offsets versus RVAs, cross-page reads, BSS slots, owned snapshots, module retention, invalid headers/protection rejection |

Test sources: [GI discovery/joystick tests](../games/gi/tests/gi_touch_tests.cpp),
[process tests](../common/process/mobile_tests.cpp),
[touch state](../games/zzz/tests/touch_state_tests.cpp),
[bridge harness](../games/zzz/tests/bridge_test_body.inc),
[elevation](../common/windows/tests/elevation_tests.cpp),
[logging](../launcher/tests/launcher_log_tests.cpp), and
[memory reader](../games/zzz/tests/memory_tests.cpp).

Bridge tests compile the production payload with `TOUCHUI_BRIDGE_TESTING=1`
into a separate object target. They exercise forwarding, source deduplication,
snapshot consistency, cancellation, native fallback, UI notifications and
owned restoration, registration, and stale compare/exchange rejection. They
do not start the full worker in an actual game or certify real event delivery
from a physical touchscreen.

`MobileUITestChild` is a fixture dependency, not a separate CTest registration.
`ProfileDiscoveryTests` is compiled by the default build but has no registered
default case; its tests need external samples.

Inspect registrations without executing tests:

```powershell
cmake --list-presets=all
ctest --preset windows-x64-release --show-only
python games/zzz/tools/generate_profiles.py --check
```

CTest enumeration requires an already configured build tree. Its output lists
tests; it does not mean those tests passed. After rebuilding, execute native
cases with `ctest --preset windows-x64-release`, or the complete script to also
cover CLI and packaged boundaries.

## External sample tests

External checks read game files as data. They do not execute or load the
supplied game binaries through the OS loader. The discovery fixtures simulate
loaded layouts using owned mappings and test buffers. Default process tests
can still create owned helper processes and execute project stubs; that is a
different operation from executing supplied game samples.

Prepare a separately supplied root with:

```text
C:/Samples/ZZZ/
  2.5/GameAssembly.dll
  2.6/GameAssembly.dll
  3.1/GameAssembly.dll
  3.2/GameAssembly.dll
```

Use a separate build and output tree to avoid confusing sample registration
with the normal default configuration:

```powershell
./scripts/full.ps1 -BuildDir build/windows-x64-samples -DistDir dist/samples -ZZZSampleRoot 'C:/Samples/ZZZ'
```

Add optional SR/GI checks when their inputs exist:

```powershell
./scripts/full.ps1 -BuildDir build/windows-x64-samples -DistDir dist/samples -ZZZSampleRoot 'C:/Samples/ZZZ' -SRSample 'C:/Samples/SR/GameAssembly.dll' -GiSample 'C:/Samples/GI/GenshinImpact.exe'
```

The optional GI oracle comparison requires the analyzed 7.1 EXE; see
[GI UI evidence](GI_TOUCH.md). Production does not use these oracle addresses.
For another EXE, use `GITouchTests --discover <exe>` for discovery acceptance
without the 7.1 oracle. Neither check establishes live touch compatibility.
The old `UserAssembly.dll` layout is not accepted by these checks.

CTest adds `ProfileDiscovery2.5`, `ProfileDiscovery2.6`,
`ProfileDiscovery3.1`, and `ProfileDiscovery3.2`, plus
`MobileUIStarRailSample` when configured. GI's optional check is run by the
script after CTest and is not a CTest case. `-SRSample` currently requires
`-ZZZSampleRoot` in the wrapper scripts; `-GiSample` does not.

The [discovery executable](../games/zzz/tests/discovery_tests.cpp) compares all
15 fields to an independently supplied manual oracle for the input hash. It
tests full file and simulated loaded views, compact fixtures, altered and
ambiguous candidates, valid dispatch permutations, ASLR bases, initialized
slots, unreadable storage, unknown fixture hashes, removed backing files, and
memory-only mutations. An unrecognized sample hash fails the test's oracle
selection even though production discovery has no hash allowlist.

Direct read-only checks on a built tree are also available:

```powershell
./build/windows-x64-release/Release/ProfileDiscoveryTests.exe 'C:/Samples/ZZZ/3.1/GameAssembly.dll'
./build/windows-x64-release/Release/MobileUITests.exe 'C:/Samples/SR/GameAssembly.dll'
./build/windows-x64-release/Release/GITouchTests.exe --sample 'C:/Samples/GI/GenshinImpact.exe'
```

For direct use of the samples preset, configure its required paths first:

```powershell
cmake --preset windows-x64-samples '-DTOUCHUI_ZZZ_SAMPLE_ROOT=C:/Samples/ZZZ'
cmake --build --preset windows-x64-samples
ctest --preset windows-x64-samples
```

This runs native sample tests only, not the complete packaging/Python suite.
Use `-DTOUCHUI_SR_SAMPLE=...` at configure time for the optional SR CTest case.

### Cached registration matters

`test.ps1` does not reconfigure CMake. Passing it a different sample root does
not retarget existing CTest commands. Those commands use paths cached when
the tree was configured. The script checks that samples exist and the external
option is on, but does not compare every cached path with its arguments.

Likewise, omitting `-ZZZSampleRoot` prints a skip message but does not remove
sample cases from a previously configured tree. A previously cached SR path
can persist if a later configure does not explicitly clear it. Inspect
`CMakeCache.txt` and CTest enumeration, use separate trees, and explicitly
configure/clear sample cache values before claiming which samples ran.

## Diagnostics and boundary checks

Run the [CLI verifier](../launcher/tests/validate_cli.py) against the built
launcher, where its `MobileUITestChild.exe` companion is present:

```powershell
python launcher/tests/validate_cli.py build/windows-x64-release/Release/TouchUILaunch.exe
```

It covers help, silence/logging, invalid flags and combinations, Unicode/space
paths, executable identity, preservation of an existing owned fixture process,
and WW behavior without the payload. The internal relaunch marker prevents
real UAC prompts. On a standard token the WW case checks the elevation guard;
on an elevated token it checks actual owned-fixture argument delivery and
survival. Record the token context if that distinction matters to a change.

Optional `--probe-game 'C:/Games/ZZZ/ZenlessZoneZero.exe'` adds positive probe
and alias checks against supplied files. This option is not used by the default
verification script and does not launch the game.

Run the [boundary verifier](../games/zzz/tools/validate_memory_boundary.py)
against freshly packaged artifacts after resolver or dependency changes:

```powershell
python games/zzz/tools/validate_memory_boundary.py --output-dir dist
```

It checks the resolver's transitive local source includes for forbidden
file/hash/path/loader dependencies; release include closures for test-oracle
leakage; one payload resolution call; launcher file probing confined to the
probe branch; the DLL's crypto imports; and known manual hash strings in the
EXE/DLL. Its JSON contains the checked sources' digests and imported DLLs.
This is a static/source and artifact check, not OS tracing or gameplay.

The CLI test uses the built EXE while the boundary check uses packaged files.
`test.ps1` checks existence, not that both locations came from the latest
sources. Build and install before using these checks to validate code changes.

Field-schema checking is part of the default suite. Scan-rule checking and
the older static evidence validator are additional workflows with different
inputs; see [Development](DEVELOPMENT.md#generated-files).

## Choose checks for a change

Commands below assume the affected Release targets have been rebuilt. Add
`./scripts/full.ps1` for changes crossing multiple modules or packaging.

| Change | Focused checks | Additional evidence |
| --- | --- | --- |
| Documentation only | Relative links/anchors, paths, command syntax, `git diff --check`; preset/test enumeration where relevant | No runtime test claims from enumeration |
| CLI, registry, family rejection, extra arguments | `python launcher/tests/validate_cli.py build/windows-x64-release/Release/TouchUILaunch.exe`; `ctest --preset windows-x64-release --no-tests=error -R '^MobileUI$'` | Update both usage READMEs when behavior changes |
| UAC/quoting | `ctest --preset windows-x64-release --no-tests=error -R '^LauncherElevation$'`; CLI checks | Actual UAC cancel/accept scenarios need manual observation if affected |
| Launcher logs | `ctest --preset windows-x64-release --no-tests=error -R '^LauncherLog$'`; CLI checks | Inspect durable elevated-session logs when that path changes |
| Active GI UI plan or initializer | `ctest --preset windows-x64-release --no-tests=error -R '^GITouch$'`; `scripts/test.ps1 -GiSample <7.1-exe>` | Read-only sample validation; manual mobile layout and touch acceptance |
| SR / retained legacy resolver or MASM | `ctest --preset windows-x64-release --no-tests=error -R '^MobileUI$'` | Relevant read-only GI/SR samples; manual per-game UI/input check |
| WW arguments/startup | `MobileUI` CTest plus CLI verifier | Manual WW acceptance for the target game build |
| ZZZ contact state | [Contact-state checks](#contact-state-checks) | Real multi-finger and streaming scenarios when affected |
| ZZZ payload/window/UI calls | [Touch-bridge checks](#touch-bridge-checks); boundary check | Frame stability, focus/capture loss, reentrancy, and manual input |
| PE reader, decoder, semantic discovery | [Reader and discovery checks](#reader-and-discovery-checks); all available `ProfileDiscovery` sample cases; boundary check | Positive variations and relevant malformed/ambiguous rejection cases |
| Generated schema or rules | Corresponding generator `--check`, rebuilt dependent tests, boundary check | Scan rules need explicit 3.1 provenance and available version regressions |
| CMake, includes, dependencies, package | `./scripts/full.ps1` and install/destination inspection | Sample configuration too if registration changed |

Register new `*_tests.cpp` targets and cases in `CMakeLists.txt`; retain
standalone executables and meaningful failure messages. No numerical coverage
threshold is configured. Add regression cases for changed behavior rather than
only duplicating implementation details.

The grouped filters below are outside the table so their `|` alternation is
identical in raw Markdown and rendered commands. Focused commands use
`--no-tests=error` so an empty selection fails. To inspect a selection without
executing tests, add `--show-only`; enumeration still does not establish a pass.

### Contact-state checks

Selects `TouchState`, `TouchBridge31`, and `TouchBridge32` (three tests):

```powershell
ctest --preset windows-x64-release --no-tests=error -R '^(TouchState|TouchBridge31|TouchBridge32)$'
```

### Touch-bridge checks

Selects `TouchBridge31` and `TouchBridge32` (two tests):

```powershell
ctest --preset windows-x64-release --no-tests=error -R '^TouchBridge(31|32)$'
```

### Reader and discovery checks

Selects `MemoryReader` and `MobileUI` (two default fixture tests). Run the
available external `ProfileDiscovery` cases separately when discovery changes:

```powershell
ctest --preset windows-x64-release --no-tests=error -R '^(MemoryReader|MobileUI)$'
```

## Manual acceptance

Use explicit game paths, the recorded EXE/DLL build, and `--log` for a planned
session. Record game version/build, Windows version, input device, display
size/scaling, and streaming software when applicable. Close the selected game
normally before using the launcher.

| Scenario | Observe |
| --- | --- |
| Launch and elevation | One game starts, cancellation performs no game action, an already-running game is preserved, launcher exit status/logs are coherent |
| GI/SR | Initialization stages complete, mobile UI visibly appears, input behaves as required, and gameplay remains usable; record each game separately |
| WW | Correct cloud arguments yield the expected UI/input behavior and the game survives launcher exit |
| ZZZ startup | Separate startup, discovery, bridge readiness, effective layout, and actual input; inspect payload log after launcher exit |
| ZZZ contacts | Single tap, hold, move, rapid tap, two or more simultaneous fingers, release ordering, and ID reuse do not leave stuck touches |
| ZZZ coordinates | Corners, resized windows, full-screen/windowed transitions, and relevant scaling remain aligned |
| ZZZ focus/capture | Focus loss and capture changes cancel active contacts without stuck input after return |
| ZZZ sources | Physical and streamed touch are checked independently; input is not duplicated when Windows supplies overlapping event forms |
| ZZZ lifecycle | Clean game exit; restart after a disabled/faulted bridge; no claim of supported DLL unloading or public disable command |

For hook-conflict, rollback, or native-fault changes, prefer controlled owned
fixtures for fault injection. Do not manufacture corruption in a real game as
an incidental acceptance test. Existing bridge fixtures do not exercise every
worker error path; add relevant coverage when changing such a path.

## Reporting results

State the commands actually executed and whether each passed, failed, or was
skipped. Include custom build/dist directories, sample identifiers, and token
context when relevant. Distinguish a missing toolchain or sample from a product
failure. Report known unverified manual scenarios plainly.

For example: “Default verification passed; external sample checks were skipped
because no inputs were supplied; no real game was launched.” Do not report
that result as verified support for all four games or all ZZZ versions.
