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
| NTE launcher relay | `logs/nte-relay-<launcher-pid>.log` beside the DLL | New child identities, role selection, verified module/startup handshake and failures |
| NTE payload | `logs/nte-<game-pid>.log` beside the DLL | Module discovery, reflection readiness and installation result; absent if loading never reached the worker |
| EF payload | `logs/ef-<game-pid>.log` beside the DLL | Attached window-thread/metadata/provider readiness, switch rejection/exception and confirmed Touch state; see [EF diagnostics](EF_INTERNALS.md#diagnostics-and-manual-acceptance) |
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
| `LoaderDiagnostics` times out with a Windows “Bad Image” dialog for `invalid image.dll` | This is the deliberately invalid test fixture. Rebuild the fixed `LoaderTests`: its owned process tree suppresses loader hard-error dialogs and checks error 193 directly. Keep the invalid-image case enabled; increasing the timeout does not fix the dialog wait. |
| Windows Terminal shows `0x800700e8` for `nte relay fixture ...\HTGame.exe ... managed.txt` | That path is the short-lived managed relay fixture. Rebuild the login host: this fixture now uses CreateNoWindow with redirected streams and verifies that it has no console, so the terminal does not race its exit. |
| NTE reports `MH_ERROR_MEMORY_ALLOC` preparing its native predicate | DLL loading already succeeded. Capture the extended payload diagnostic: nearby executable-buffer/private-heap preparation failed. A later successful run does not establish a fix; do not attribute the failure to account login or assume a touch configuration argument caused it. |

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
the actual executable filename, and whether an option is supported. Use one
`--extra <arguments>` value to append game arguments, quoting the complete
value when it contains spaces. WW's cloud flags are internally supplied before
that optional extra string. Use `--log` to see validation errors.

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
terminates the still-owned child. After successful release, `Child` no longer
terminates it. This distinction is covered by
[process tests](../common/process/mobile_tests.cpp). EF still requires its
controller to retain the owned official parent until game exit; keep it running.

## EF startup and intermittent UI clicks

Start with the [Endfield setup](../README.md#endfield-setup-and-current-status)
or [中文启动说明](../README.zh-CN.md#终末地启动与验证状态). Verify `--help` includes
`--EF` and `--ef-launcher`, and keep the EXE/DLL from the same build together.
An older release may not include the current startup or pointer handling.

| Symptom or last marker | Interpretation and next step |
| --- | --- |
| `elevated launcher exited with code ...` | The outer launcher is reporting the elevated process result. Inspect the second `launcher-<pid>.log` for its stage/error; the numeric code alone does not establish a UAC or payload cause. |
| `trusted GRYPHLINK Games.exe not found` | Auto-discovery checks `%ProgramFiles%/GRYPHLINK` and immediate numeric-version folders. For a custom installation, use `--ef-launcher <installed Games.exe>`. |
| Signature, product or x64 parent rejection | Select the actual installed signed x64 GRYPHLINK launcher. Its trust/product check is repeated at creation; renaming another EXE does not satisfy it. The optional [read-only parent check](TESTING.md#endfield-checks) isolates this from game startup. |
| “请通过官方启动器打开游戏” / “start through the official launcher” | The direct-start build lacked the required official parent. The current route creates a verified owned parent and loads the DLL before game main; check the actual EXE/DLL and `EF official parent` log path. No manual Play click is required. |
| `Payload module was not loaded at the expected path` or old `EF: load layout-only payload` stage | These were observed on the superseded late-load route. Use the current early-load build. A current `Direct LoadLibraryW did not map expected DLL` error includes loader-thread and child-process exit values; collect both and the preload stage. |
| Wait for metadata or DeviceInfo provider/notification | Startup is pending. Inspect the EF payload's elapsed time and real input/provider/notification values. Required contract/export failures are explicit; do not assume a fixed-address mismatch or claim Ready from a loaded DLL. |
| Layout Ready but no pointer application report | Layout and pointer handling have separate evidence. Check `independent mouse/touch policy ready` and any unavailable/failure message. An already-independent instance or the eight-report limit can also explain no new application report. |
| Pointer routing unavailable or field readback failed | Layout can still work. The pointer capability remains unavailable/pass-through for that session; retain the exact error and restart normally with a compatible build for a new attempt. No runtime retry toggle exists. |
| Menu/story taps show only hover feedback | The independent pointer policy targets this reported symptom, but real-game improvement remains unverified. Follow the [manual retest cases](EF_INTERNALS.md#manual-retest), recording screen/control, mouse/touch sequence, attempts/failures and logs. |
| Controller stays open after `Endfield touch UI state confirmed` | Expected: it retains the official parent until game exit. Do not close it as a way to finish initialization. Its eventual `0` exit does not report the game's exit code. |

In the historical late-load diagnostic, `phase=0` with loader thread status
`0xDEADC0DE` meant the diagnostic entry marker was never reached, before
LoadLibrary was called. It did not identify an IL2CPP signature failure. The
current route uses normal early `LoadLibraryW`, with no late-load fallback.

Preserve the outer/elevated launcher logs and `logs/ef-<game-pid>.log` from the
same attempt. New DLLs take effect after a normal exit/relaunch, not in an
already-running session. Existing official launcher processes are not injected
or terminated by this path. For exact log meanings and degradation behavior,
see [EF readiness](EF_INTERNALS.md#readiness-and-fallback).

## NTE payload loading failures

An outer `elevated launcher exited with code 2` means the elevated launcher's
own log contains the failing stage. Default `--NTE` startup uses the independent
SDK host, with a creation relay that early-loads the touch DLL in its new game.
The older bare direct path could show UI Ready without a login session; its
Enter Game click sound was not an input failure. The public route now supplies
the full regional login host. UI Ready still does not prove authentication.

For independent login, keep `nte-login-host/` intact, select `--nte-region CN`
or `--nte-region Global`, and exit existing games/platform processes first.
Missing/unknown SDK pairs fail closed. The host's sanitized startup diagnostics
are under `%LocalAppData%\TouchUILaunch\NTE\Diagnostics\`: inspect fixed
phase and failure keys, not account files. `sdk-environment-acknowledged`,
`sdk-registration-validated`, `login-requested`, and
`authenticated-identity-delivered` distinguish platform startup from login.
Use `--nte-official` for the official relay while an SDK profile is unverified
or unsupported. There is no automatic fallback to a different login route.

`native bool prologue is unsupported` occurs after DLL mapping, during UI
discovery. Its RVA and bounded `entry` bytes identify an unrecognized native
entry, not an injection failure. Current support handles the observed Global
stack-only entry and CN saved-RBX frame via different publication methods;
other forms remain rejected. Restart with an adapted build; do not reinject or
skip the entry validation in an already running game.

The direct loader verifies the full module path. Its thread exit value truncates
a module handle and can contain a termination status, so nonzero never proves
loading succeeded. Unlike the older diagnostic stub, this path does not capture
a same-thread `GetLastError` from a failed remote LoadLibrary call.

The prior direct-game experiment recorded phase `0` and thread status
`0xDEADC0DE`, before the diagnostic stub entered. It did not reach DLL loading or
UI code. That result describes the previous late-loading route, not a successful
or failed test of the official-launcher relay. Process protections are not
altered. Report the current stage, relay/worker logs and actual game exit code
before attributing a failure to the loader, static discovery or UI behavior.

A failed setup closes only the newly created launcher/game tree. An unrelated
existing process is not adopted. On successful initialization the controller
releases the job and the game survives its exit. Afterward relay hooks remain
resident but forward calls without further DLL propagation.

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
