# Architecture

TouchUILaunch is a Windows x64 C++20 launcher with per-game mobile UI paths and
a separate ZZZ touch DLL. The production package has two program artifacts:
`TouchUILaunch.exe` and `TouchUILaunch.dll`. Their shared filename stem does not
mean every game loads the DLL.

Read [Game support](GAME_SUPPORT.md) for CLI details,
[ZZZ internals](ZZZ_INTERNALS.md) for resolver and bridge mechanics, and
[Development](DEVELOPMENT.md) for build commands.

## Components and ownership

| Component | Owns | Useful entry points |
| --- | --- | --- |
| Launcher | CLI parsing, static game registry, diagnostics, elevation coordination, launch serialization, ZZZ injection | [launcher.cpp](../launcher/src/launcher.cpp), [game.hpp](../launcher/include/game.hpp), [launcher_log.hpp](../launcher/include/launcher_log.hpp) |
| GI adapter and resolver | GI entry adapter; the current shared GI/SR signature resolver; GI initialization stub | [gi_mobile.hpp](../games/gi/include/gi_mobile.hpp), [mobile_resolver.hpp](../games/gi/include/mobile_resolver.hpp), [gi_stubs.asm](../games/gi/src/gi_stubs.asm) |
| SR adapter | SR entry adapter and repeated UI-state worker | [sr_mobile.hpp](../games/sr/include/sr_mobile.hpp), [sr_stubs.asm](../games/sr/src/sr_stubs.asm) |
| WW | Android cloud UI launch arguments | [ww_launch.hpp](../games/ww/include/ww_launch.hpp) |
| ZZZ | Memory discovery, optional file probe, injected input/UI bridge, fixtures and generators | [profile_resolver.hpp](../games/zzz/include/profile_resolver.hpp), [payload.cpp](../games/zzz/src/payload.cpp) |
| Windows helpers | RAII handles, module/address helpers, event names, elevation and argument quoting | [win_util.hpp](../common/windows/include/win_util.hpp), [elevation.hpp](../common/windows/include/elevation.hpp) |
| Binary helpers | PE section/RVA access, masked patterns, bounded x64 decoding | [pe_image.hpp](../common/binary/include/pe_image.hpp), [x64_reader.hpp](../common/binary/include/x64_reader.hpp), [pattern.hpp](../common/binary/include/pattern.hpp) |
| Process helpers | Remote memory operations, loader bootstrap, GI/SR installation and contexts | [mobile_runtime.hpp](../common/process/include/mobile_runtime.hpp), [load_library.asm](../common/process/src/load_library.asm) |
| Test helpers | Owned child executable and simulated module mappings | [mobile_test_child.cpp](../common/testing/mobile_test_child.cpp), [test_module.hpp](../common/testing/include/test_module.hpp) |

Most implementation modules are headers, consumed through explicit local
include directories. A public header in this project is not necessarily an
externally supported library API. Keep its callers, binary layout assumptions,
and CMake consumers in view when changing it.

## Current dependencies and maintenance direction

The intended direction is a generic launcher, per-game behavior, and reusable
common utilities. Keep new shared utilities independent of game modules and
keep game signatures out of the launcher. The current implementation has
concrete exceptions:

- `common/process/include/mobile_runtime.hpp` includes the launcher-owned
  `game.hpp` and the GI-owned `mobile_resolver.hpp`.
- That process header implements both GI and SR installation and branches on
  `game::Kind`. The GI/SR adapters delegate to it.
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
| `RemoteLoaderStubs`, `GIMobileStubs`, `SRMobileStubs` | Separate static MASM blocks |
| `TouchUIProcess` | Header implementation plus decoder, stubs, `user32`, and `psapi` |
| `TouchUIGI`, `TouchUISR`, `TouchUIWW` | Interface targets connecting game adapters to their dependencies |
| `TouchUIZZZDiscovery` | Header discovery implementation and HDE64 |
| `TouchUILaunch` | Launcher EXE; also compiles the explicit ZZZ file diagnostic through included headers |
| `TouchUILaunchPayload` | Production DLL, output name `TouchUILaunch`; ZZZ discovery and `user32` |
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
2. Reject an already-running process of the selected game family. For ZZZ,
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
| GI | Suspended main thread; initialize loader, capture target module, resolve and install initialization hook | After successful setup and main-thread resume |
| SR | Suspended main thread; load/capture GameAssembly, resolve UI state, start repeated writer | After worker startup and main-thread resume |
| WW | Normal process with cloud UI arguments | Immediately after process creation |
| ZZZ | Normal process; wait for GameAssembly, inject adjacent DLL, observe startup event | After the payload worker publishes its startup event |

`game::Child` closes its process/thread handles on destruction. Until
`release()`, its destructor also terminates the process it created and waits
briefly for exit. After release, the game survives launcher exit. Never wrap a
discovered PID in this ownership abstraction.

The running-game check uses accepted executable names, including GI's two
names, rather than only the selected installation path. The mutex coordinates
launcher copies of the same game type; it does not constitute a universal
lock against unrelated external launchers.

## Remote memory and callback ownership

GI/SR remote addresses are rebased from the actual module that owns a system
API in the child. A suspended process may not yet have a Toolhelp module list,
so the process layer inspects mapped images. It validates architecture,
allocation ownership, protections, and complete memory transfers.

`RemoteBlock` separates code and context into two pages. Code becomes
executable/readable after installation; context remains writable. Once a live
thread or installed callback can reference a block, retain it. In particular,
a loader timeout must not free memory that the remote thread may still use.
GI C++ context offsets must match MASM operands, enforced partly by
`static_assert` declarations and [MobileUI tests](../common/process/mobile_tests.cpp).

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

Similarly, GI hook installation does not prove the target initialization
function has run, SR worker startup does not prove visible UI, and WW process
creation does not prove the game honored its arguments. Use
[evidence levels](TESTING.md#evidence-levels) when reporting results.
