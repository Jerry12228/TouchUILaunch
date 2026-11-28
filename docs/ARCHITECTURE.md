# Architecture

TouchUILaunch is a Windows x64 C++20 launcher with per-game mobile UI paths.
The base package has two program artifacts: `TouchUILaunch.exe` and the shared
`TouchUILaunch.dll` used by ZZZ, NTE and EF. Optional NTE direct login adds a
self-contained .NET/Avalonia SDK host under `nte-login-host/`; its native relay
loads the shared DLL only in newly created, configured processes.

Read [Game support](GAME_SUPPORT.md) for CLI details,
[ZZZ internals](ZZZ_INTERNALS.md) for resolver and bridge mechanics, and
[Development](DEVELOPMENT.md) for build commands.

## Components and ownership

| Component | Owns | Useful entry points |
| --- | --- | --- |
| Shared payload entry | One DllMain/dispatcher; select exactly one coordinated game worker | [payload.cpp](../payload/src/payload.cpp), [payload_entry.hpp](../payload/include/payload_entry.hpp) |
| Launcher | CLI parsing, static game registry, diagnostics, elevation coordination, launch serialization, ZZZ injection | [launcher.cpp](../launcher/src/launcher.cpp), [game.hpp](../launcher/include/game.hpp), [launcher_log.hpp](../launcher/include/launcher_log.hpp) |
| Retained legacy GI / SR resolver | Shared legacy signature resolver and stubs, still needed by SR and legacy fixtures | [gi_mobile.hpp](../games/gi/include/gi_mobile.hpp), [mobile_resolver.hpp](../games/gi/include/mobile_resolver.hpp), [gi_stubs.asm](../games/gi/src/gi_stubs.asm) |
| SR adapter | SR entry adapter and repeated UI-state worker | [sr_mobile.hpp](../games/sr/include/sr_mobile.hpp), [sr_stubs.asm](../games/sr/src/sr_stubs.asm) |
| WW | Android cloud UI launch arguments | [ww_launch.hpp](../games/ww/include/ww_launch.hpp) |
| ZZZ | Memory discovery, optional file probe, injected input/UI bridge, fixtures and generators | [profile_resolver.hpp](../games/zzz/include/profile_resolver.hpp), [payload.cpp](../games/zzz/src/payload.cpp) |
| NTE | Owned official/independent SDK-host relay, regional login, UE discovery, mobile predicates/navigation and native touch/paging corrections | [nte_discovery.hpp](../games/nte/include/nte_discovery.hpp), [nte_input.hpp](../games/nte/include/nte_input.hpp), [nte_pager.hpp](../games/nte/include/nte_pager.hpp), [payload.cpp](../games/nte/src/payload.cpp), [NTE internals](NTE_INTERNALS.md) |
| EF | Verified owned official parent, early DLL load, IL2CPP contracts, window-thread input switch and independent UI pointers | [ef_parent.hpp](../games/ef/include/ef_parent.hpp), [ef_runtime.hpp](../games/ef/include/ef_runtime.hpp), [ef_pointer.hpp](../games/ef/include/ef_pointer.hpp), [EF internals](EF_INTERNALS.md) |
| Windows helpers | RAII handles, module/address helpers, event names, elevation and argument quoting | [win_util.hpp](../common/windows/include/win_util.hpp), [elevation.hpp](../common/windows/include/elevation.hpp) |
| Binary helpers | PE section/RVA access, masked patterns, bounded x64 decoding | [pe_image.hpp](../common/binary/include/pe_image.hpp), [x64_reader.hpp](../common/binary/include/x64_reader.hpp), [pattern.hpp](../common/binary/include/pattern.hpp) |
| Process helpers | Remote memory operations, loader bootstrap, GI/SR installation and contexts | [mobile_runtime.hpp](../common/process/include/mobile_runtime.hpp), [load_library.asm](../common/process/src/load_library.asm) |
| Test helpers | Owned child executable and simulated module mappings | [mobile_test_child.cpp](../common/testing/mobile_test_child.cpp), [test_module.hpp](../common/testing/include/test_module.hpp) |

Most implementation modules are headers, consumed through explicit local
include directories. A public header in this project is not necessarily an
externally supported library API. Keep its callers, binary layout assumptions,
and CMake consumers in view when changing it.

The active GI entry is [gi_touch.hpp](../games/gi/include/gi_touch.hpp),
with [signature discovery](../games/gi/include/gi_touch_plan.hpp) and
[leaf joystick helper](../games/gi/include/gi_joystick_code.hpp). It uses generic
PE/x64/Windows helpers, HDE64 and child ownership, with no version hash gate.
See [GI touch](GI_TOUCH.md) for discovery, scale evidence and helper lifetime.
It has no dependency on the legacy process initializer or GI stubs.

## Current dependencies and maintenance direction

The intended direction is a generic launcher, per-game behavior, and reusable
common utilities. Keep new shared utilities independent of game modules and
keep game signatures out of the launcher. The current implementation has
concrete exceptions:

- `common/process/include/mobile_runtime.hpp` includes the launcher-owned
  `game.hpp` and the GI-owned `mobile_resolver.hpp`.
- That process header implements both GI and SR installation and branches on
  `game::Kind`. The SR adapter delegates to it; the active GI adapter does not.
- The GI-owned resolver contains SR signatures as well as GI signatures.
- `TouchUIHeaders` exposes all local include roots. The CMake interface targets
  describe composition; they do not enforce strict header isolation.
- Common PE/x64 readers use the `discovery` namespace even though their files
  are shared infrastructure.

These are current dependencies, not instructions to duplicate them or claims
that layering is already complete. A dependency refactor must be an intentional
task with its own compatibility checks. Do not perform one incidentally while
editing a guide or adding a small fix.

## Build and runtime composition

[CMakeLists.txt](../CMakeLists.txt) is the target source of truth:

| Target/group | Role |
| --- | --- |
| `TouchUIHeaders` | Interface include surface for project headers |
| `Hde64` | Bundled static x64 decoder |
| `MinHook` | Pinned v1.3.4 static hooks for NTE paths and EF input-switch/UI-pointer guards; separate decoder symbols |
| `RemoteLoaderStubs`, `GIMobileStubs`, `SRMobileStubs` | Separate static MASM blocks |
| `TouchUIProcess` | Header implementation plus decoder, stubs, `user32`, and `psapi` |
| `TouchUIGI`, `TouchUISR`, `TouchUIWW` | Interface targets connecting game adapters to their dependencies |
| `TouchUIZZZDiscovery` | Header discovery implementation and HDE64 |
| `TouchUILaunch` | Launcher EXE; also compiles the explicit ZZZ file diagnostic through included headers |
| `TouchUILaunchPayload` | Shared production DLL: one dispatcher, ZZZ bridge, NTE worker/relay, EF layout worker, MinHook and owned loader helpers |
| `TouchUINTE` | Header interface for NTE discovery/callback fixtures |
| `TouchUIZZZBridgeTestHarness` | Separate object build of the payload with `TOUCHUI_BRIDGE_TESTING=1` |

Test executables and CTest registrations are described in [Testing](TESTING.md).
The bridge harness includes internal fixture assertions in the production
translation unit only for the test object build. The release DLL must not
include the test oracle header. The launcher uses `bcrypt` for explicit file
diagnostics; the payload must not acquire diagnostic crypto imports.

## Launch and process lifetime

The common path in [wmain](../launcher/src/launcher.cpp) is:

1. Detect `--log` early, open optional diagnostics, and parse/validate selection
   and paths. Help returns directly. A ZZZ probe uses a separate read-only path.
2. Reject an already-running process of the selected game family. For ZZZ/NTE/EF,
   check that the adjacent payload exists.
3. If necessary, relaunch the launcher with Windows elevation. The original
   launcher waits and returns the elevated child's exit code; it does not
   also start the game.
4. Acquire the per-game named launch mutex with a nonblocking wait and repeat
   the running-process check.
5. Create a game process with `game::Child`, perform game-specific setup, and
   release ownership when that setup reaches its launcher-side success point.

| Game | Creation and setup | Point at which the launcher releases its child |
| --- | --- | --- |
| GI | Discover eleven UI/input/joystick windows from the file and loaded EXE; install validated patches and an owned joystick helper while suspended | After successful setup and main-thread resume |
| SR | Suspended main thread; load/capture GameAssembly, resolve UI state, start repeated writer | After worker startup and main-thread resume |
| WW | Normal process with cloud UI arguments | Immediately after process creation |
| ZZZ | Normal process; wait for GameAssembly, inject adjacent DLL, observe startup event | After the payload worker publishes its startup event |
| NTE | Bundled regional SDK/login host by default; --nte-official selects the suspended official launcher relay; bridge early-loads the newly created game DLL, then waits for UnrealWindow and installs UI hooks | After successful UI installation; failure/cancellation cleans the newly created launcher/game job |
| EF | Verified owned GRYPHLINK parent and suspended game; map DLL before main, resume and resolve metadata on attached window thread | Release game ownership after Touch readback, then keep official parent/controller alive until game exit; initialization failure/timeout cleans only owned processes |

`game::Child` closes its process/thread handles on destruction. Until
`release()`, its destructor also terminates the process it created and waits
briefly for exit. After release, the game survives launcher exit. Never wrap a
discovered PID in this ownership abstraction.

The running-game check uses accepted executable names, including GI's two
names, rather than only the selected installation path. The mutex coordinates
launcher copies of the same game type; it does not constitute a universal
lock against unrelated external launchers.

## Remote memory and callback ownership

The retained legacy GI/SR helper remote addresses are rebased from the actual module that owns a system
API in the child. A suspended process may not yet have a Toolhelp module list,
so the process layer inspects mapped images. It validates architecture,
allocation ownership, protections, and complete memory transfers.

`RemoteBlock` separates code and context into two pages. Code becomes
executable/readable after installation; context remains writable. Once a live
thread or installed callback can reference a block, retain it. In particular,
a loader timeout must not free memory that the remote thread may still use.
Retained legacy GI C++ context offsets must match MASM operands, enforced partly by
`static_assert` declarations and [MobileUI tests](../common/process/mobile_tests.cpp).

NTE's [diagnostic loader](../common/process/include/load_library_diagnostic.hpp)
uses a separate additive MASM entry to capture full `HMODULE`, completion and
same-thread loader error. Its code and writable context/path occupy separate
pages; a wait timeout retains the allocation until the owned child exits.
The existing GI/SR loader entry and ZZZ loading path are unchanged.

ZZZ's resolver pins GameAssembly for process lifetime. The payload's installed
callbacks and window procedure must remain resident until process exit; there
is no supported unload or reattach lifecycle. Hook slot replacement uses
compare/exchange with the expected previous pointer. See
[hook ownership](ZZZ_INTERNALS.md#hook-installation-and-failure-handling).

## Discovery, diagnostics, and evidence boundaries

The ZZZ runtime entry `discovery::resolve_module` reads the loaded module and
calls shared discovery logic once. The explicit file probe uses that discovery
logic on a read-only disk view and computes a diagnostic SHA-256. It does not
select a fixed version profile.

The generated production `profile::Build` is a field schema without client
addresses or fingerprints. Versioned JSON, IDA listings, and generated
`test_profiles.hpp` are development evidence. They support independent
comparisons in tests, not runtime address lookup.

Memory boundaries are checked at source-dependency and binary-import level by
[validate_memory_boundary.py](../games/zzz/tools/validate_memory_boundary.py).
This check is not OS file-I/O tracing and does not imply that optional logging
is prohibited. See [Testing](TESTING.md#diagnostics-and-boundary-checks).

## Success boundaries

A launcher exit code of zero means its own path completed. For ZZZ, the startup
event is published before resolution and hook installation, so later worker
failures can occur after that exit. Even the payload's `READY` message still
awaits a usable UI provider and input.

Similarly, successful GI patch installation does not prove the target initialization
function has run, SR worker startup does not prove visible UI, and WW process
creation does not prove the game honored its arguments. Use
[evidence levels](TESTING.md#evidence-levels) when reporting results.

NTE waits for installation rather than a worker-start event, but visible HUD,
input mappings, and touch delivery still require manual acceptance. Its
[process-lifetime callbacks](NTE_INTERNALS.md#installation-and-lifetime) remain
pinned and use the game's original navigation setter on its calling thread.

EF also waits for state confirmation. Its loader worker does not attach itself
to IL2CPP or invoke managed code. A USER32 timer dispatches on the Unity window
thread, verifies the thread is already attached, resolves its metadata contract
and invokes the game's setter. The input-switch detour starts as pass-through
and enforces Touch only after successful readback. See [EF internals](EF_INTERNALS.md)
for initialization gates and manual acceptance limits.

EF pointer hooks apply Unity's AllPointersAsIs setting before routing/Process
through metadata-resolved instance-field APIs. They retain no instance across
frames. This capability degrades independently when unavailable. Official
parent verification links WinTrust/version APIs into the EXE, not the payload.
