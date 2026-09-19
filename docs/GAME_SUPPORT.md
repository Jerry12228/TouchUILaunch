# Game support and launch behavior

The [launcher](../launcher/src/launcher.cpp) and
[registry](../launcher/include/game.hpp) define the current command interface.
These paths describe implemented behavior, not a guarantee for every current
or future client build. Validation scope is defined in [Testing](TESTING.md).

## Game selection

| Flag | Accepted executable filename, case-insensitive | Explicit `--game` | Implementation |
| --- | --- | --- | --- |
| `--GI` | `YuanShen.exe` or `GenshinImpact.exe` | Required | Suspended startup and initialization hook |
| `--SR` | `StarRail.exe` | Required | Suspended startup and periodic UI-state writer |
| `--ZZZ` | `ZenlessZoneZero.exe` | Optional | Normal startup, injected DLL, memory discovery, Windows touch bridge |
| `--WW` | `Client-Win64-Shipping.exe` | Required | Normal startup with Android cloud UI arguments |

Choose exactly one game; duplicate selectors, even for the same game, are
rejected. The file must exist and its filename must match the selected family.
Paths containing spaces or non-ASCII characters are supported by the Unicode
launcher and must be quoted as one argument.

ZZZ's implicit search is a development convenience. The launcher derives a
base directory two levels above its own executable directory and tries:

1. `Client/3.2/ZenlessZoneZero.exe`.
2. `Client/ZenlessZoneZero Game/ZenlessZoneZero.exe`.
3. `ZenlessZoneZero Game/ZenlessZoneZero.exe`.

This is not an installation registry scan or a requirement that the standalone
repository contain those directories. Prefer explicit `--game` in reproducible
commands and distribution instructions.

## CLI contract

| Option | Behavior |
| --- | --- |
| `--game <exe>` | Canonicalize the selected executable path; duplicate occurrences are rejected |
| `--extra <arguments>` | Append one raw argument string to the selected game; duplicate occurrences are rejected |
| `--log` | Enable console and launcher file diagnostics; also enable the ZZZ payload log |
| `--ZZZ --probe` | Read-only GameAssembly file diagnostic and payload-existence check; no launch, injection, or elevation |
| `--probe-auto` | Compatibility alias for `--probe`, restricted to ZZZ |
| `--help`, `-h` | Print usage without requiring a game or elevation |

Only one probe action is allowed. Probe with GI/SR/WW is rejected. Unknown or
incomplete options fail. `--extra` consumes exactly one following launcher
argument without parsing its contents; quote that value when it contains spaces.
It is accepted by the read-only ZZZ probe but is not used because no game is
started. The WW arguments are supplied internally by its module before any
extra arguments.

`--status`, `--enable`, `--disable`, and `--pid` are unsupported. Internal
bridge enable/disable state does not expose corresponding public commands.
`--elevation-relaunch` is an internal loop-prevention marker, not a way to
bypass administrator checks or a recommended user option.

Without `--log`, normal launch and validation failures are silent on stdout and
stderr; help still prints. Caught launcher exceptions return `2`. Successful
help, probe, and launcher setup return `0`. Elevation can return Windows
cancellation or the elevated launcher's exit code. Consumers must check exit
status instead of inferring success from silence.

The [CLI verifier](../launcher/tests/validate_cli.py) protects flag handling,
default silence, invalid combinations, family rejection, and WW fixture
behavior. Preserve its `ERROR:` diagnostic expectations when changing logs.

## Launch, elevation, and diagnostics

Before creating a game, the launcher rejects any already-running process with
an accepted filename in that family. This also covers another installation
path. It requests UAC once if needed, obtains a per-game named mutex, then
checks again. WW currently requires elevation too, despite using no injection.

[Elevation helpers](../common/windows/include/elevation.hpp) encode Windows
CRT arguments, preserve working directory and normalized game path, and
propagate the child exit code. Failure to obtain an elevated token after a
relaunch stops the operation rather than prompting indefinitely.

The launched game's working directory is its executable's directory.
Only the newly created process is owned for failure cleanup; an existing game
is neither attached to nor terminated. See
[process lifetime](ARCHITECTURE.md#launch-and-process-lifetime).

With logging enabled, files reside beside the executable/DLL under `logs/`:
`launcher-<launcher-pid>.log` and, for ZZZ, `touch-<game-pid>.log`.
An elevated relaunch has a different launcher PID. The payload retains the
logging event after the launcher exits. See [Troubleshooting](TROUBLESHOOTING.md#collect-the-right-diagnostics).

For an intentional manual game session, use one applicable command below;
these are real launches, not automated documentation checks:

```powershell
./dist/TouchUILaunch.exe --GI --game 'C:/Games/GI/GenshinImpact.exe' --log
./dist/TouchUILaunch.exe --SR --game 'C:/Games/SR/StarRail.exe' --log
./dist/TouchUILaunch.exe --ZZZ --game 'C:/Games/ZZZ/ZenlessZoneZero.exe' --log
./dist/TouchUILaunch.exe --WW --game 'C:/Games/WW/Client/Binaries/Win64/Client-Win64-Shipping.exe' --log
./dist/TouchUILaunch.exe --GI --game 'C:/Games/GI/GenshinImpact.exe' --extra '-screen-width 1920 -screen-height 1080' --log
```

## GI and SR initialization

The [GI adapter](../games/gi/include/gi_mobile.hpp) and
[SR adapter](../games/sr/include/sr_mobile.hpp) call `mobile::initialize` in
[mobile_runtime.hpp](../common/process/include/mobile_runtime.hpp). Both create
the main thread suspended, bootstrap the child's loader using a short remote
thread, resolve from captured child memory, install behavior, then resume and
release the child. Their code does not load `TouchUILaunch.dll`.

### GI

GI first captures its main executable. If it has an `il2cpp` section, that
image supplies the signatures. Otherwise the launcher loads the legacy
`<executable-stem>_Data/Native/UserAssembly.dll` and captures that module.

The [resolver](../games/gi/include/mobile_resolver.hpp) decodes UI/input setters,
class storage, object offsets, and the initialization target. Repeated matching
sites are acceptable only when their decoded targets agree. Do not replace
that agreement check with a first occurrence or require one raw occurrence:
the source explicitly accommodates repeated GI call sites. Function targets
must differ, fields must be aligned/in range, and an already-hooked
initialization entry is rejected.

`install_gi` saves 16 original bytes and installs an indirect jump into an
owned remote code block. The [MASM stub](../games/gi/src/gi_stubs.asm) coordinates
restoration, flushes instructions, restores page protection, and calls the
original function with preserved arguments. The winning invocation then calls
the UI setter with `(object, 0, 1)` and input setter with `(object, 0, 0)` if
both objects are ready. It preserves the original return values and records
outcome bits. It is not a trampoline that executes copied prologue bytes.

`GiContext` offsets and size are asserted against the assembly layout. Changes
to argument handling, concurrency, or context members require coordinated
C++/MASM review and [MobileUI tests](../common/process/mobile_tests.cpp).
The launcher reports hook installation before the game necessarily invokes
that initialization function; it does not certify the eventual UI result.

### SR

SR explicitly loads `GameAssembly.dll` beside the game executable, captures
the loaded image, and resolves a writable UI-state location. Its resolver
supports three signature variants and requires exactly one match across them;
the matching original instruction writes state `3`.

The installed [SR worker](../games/sr/src/sr_stubs.asm) writes state `2` every
500 ms. Setup waits for its `started` flag before resuming the main thread.
`SrContext` has `target`, `sleep`, `stop`, and `started` fields shared with MASM.
The internal stop flag is not exposed as a user command. Worker startup is
evidence of installation, not visible UI or touch behavior.

The adaptation's provenance and excluded upstream features are documented in
[third_party/gi_sr](../third_party/gi_sr/README.md). Neither GI nor SR receives
ZZZ's Windows multitouch bridge through these paths. Test their actual input
behavior separately rather than promising ZZZ-equivalent touch handling.

## WW launch

[ww_launch.hpp](../games/ww/include/ww_launch.hpp) supplies exactly
`-CloudGame -CloudGamePlatform=Android`. The launcher appends the optional
`--extra` string after those flags, starts WW normally, and releases its child
immediately. This path has no suspended initialization, memory patch, or
payload dependency.

The CLI fixture verifies argument delivery, working directory, and child
survival on an elevated run, or the elevation guard on a standard-token run.
Only a real WW session establishes whether a specific build honors the flags.

## ZZZ launch and probe

For launch, the EXE waits up to roughly 120 seconds for the new child's
GameAssembly module, checks process identity and unexpected payload presence,
and injects the adjacent DLL through `LoadLibraryW`. It verifies the actual
loaded module path instead of treating a truncated thread exit code as a
64-bit module handle. A loader wait timeout retains its remote argument buffer.

The launcher then looks for the payload startup event for roughly 20 seconds.
The worker creates that event before discovery and hook installation. After
the event appears, initialization continues asynchronously inside the game.
See [ZZZ lifecycle](ZZZ_INTERNALS.md#initialization-and-readiness).

For a read-only file diagnostic with an explicit installed/supplied game path:

```powershell
./dist/TouchUILaunch.exe --ZZZ --probe --game 'C:/Games/ZZZ/ZenlessZoneZero.exe' --log
```

Probe maps the neighboring `GameAssembly.dll` as data, computes SHA-256, runs
automatic discovery, and checks that the adjacent payload exists. `--log`
controls output, including the JSON result; the probe computes the diagnostic
hash even without logging. It does not select by hash, start the game, or
establish runtime compatibility.

## Adding or changing game support

1. Define the requested launch behavior and executable identity. Update
   `game::Kind`, its descriptor registry, selection/validation messages, and
   help when introducing a new game.
2. Place signatures, arguments, and initialization semantics in the game
   module. Add the adapter and required CMake target/include/link wiring.
   Audit the launcher's current branches explicitly: an unhandled kind must
   not accidentally fall through to ZZZ setup or a GI/SR helper branch.
3. Define creation mode, setup success, and failure ownership. Preserve
   family-level running checks, UAC forwarding, and child cleanup.
4. Add owned fixtures for positive behavior and failures. Update CLI cases,
   registration, and packaging checks where affected. Add external samples
   separately if signatures need regression evidence.
5. Update both user READMEs, this chapter, architecture routing, and test
   guidance. Report real-game acceptance independently of automated coverage.

A new ZZZ client sample normally calls for discovery/evidence work rather than
a new game selector or a production version table. See
[version adaptation](ZZZ_INTERNALS.md#evidence-and-version-adaptation).
