# NTE mobile UI and independent login

NTE support enables the client's existing mobile UI and Touch navigation.
The official relay and independent SDK host both load the shared payload before
the newly created game's main thread runs. CN independent login/world/basic
touch and Global official-relay HUD/basic operation have user acceptance from
2026-11-27. Global independent-host login was also confirmed. Detailed evidence
and the limits of each result are recorded below.
The packed third-party NTETool DLL's consumer path remains unrecovered; this
implementation follows independently observed client behavior.

## Discovery and scope

[nte_image.hpp](../games/nte/include/nte_image.hpp) captures the loaded main
module once as an immutable RVA-layout image, including data and exception
tables. Unreadable memory fails without a protection bypass or file fallback.
[nte_discovery.hpp](../games/nte/include/nte_discovery.hpp) finds:

1. UE native `{name, exec}` registrations, their registrars, and owner class
   getters validated by UTF-16 class names. Chained unwind records recover a
   registrar's start when a reference lies in its secondary runtime region.
2. Generated `{constructor, name}` registrations and their lazy caches. The
   two table directions are deliberately different. Live UFunction callback,
   Outer, and UClass cache must identify exactly one object per owner/name.
3. Native `HTOneSDKUtil.IsMobileCloudGameOperation`'s single direct predicate
   call, plus the three bool exec entries owned by `HTOneSDKUtil`, `HTUtil`,
   and `HTUINavigationSubsystem`. Reflection must report a native, no-argument
   bool return (NumParms includes that return).
4. The NTE navigation class constructor and primary vtable. The setter must
   match the observed lock-check override and base read/write relationship.

Production contains no version RVA table or client fingerprint. It does retain
explicit UE field/virtual-slot ABI assumptions (`UFunction::Func +0xD8`,
Outer `+0x20`, navigation setter slot `+0x3A0`, and state relationships
`+0x148`/`+0x14B`). These require semantic validation; this is not a general
Unreal version adapter. Unsupported or ambiguous evidence fails.

## Installation and lifetime

The shared [payload entry](../payload/src/payload.cpp) checks existing per-PID
activation objects on a worker thread. NTE requires its Ready/Failed pair;
ZZZ requires its logging configuration object (which exists even with logging
disabled). Mixed or partial activation is rejected before starting a game
worker. NTE retains its own session/path/role validation. The shared module
starts only one worker and retains CRT/TLS thread notifications used by ZZZ.
The independent SDK login host remains a separate process/directory.

The launcher creates a new official launcher suspended and loads the adjacent
`TouchUILaunch.dll` directly with a `LoadLibraryW` remote thread. It resolves
the actual API host image in the owned child, including before the initial
loader list exists, and verifies the loaded module's full path. The remote
thread's truncated exit code is diagnostic only. If only ntdll is initially
mapped, a normal `RtlExitUserThread` bootstrap initializes the process loader.
No executable remote stub is used by this relay path.

The same DLL has a bridge role in `NTEGlobalLauncher.exe`, `NTEGlobalGame.exe`,
and `NTEGame.exe`, restricted to the configured installation directory. A pinned
MinHook v1.3.4 trampoline intercepts `kernel32!CreateProcessW`. It calls the
original with `CREATE_SUSPENDED`, classifies the actual new child's full path,
and propagates the DLL to qualifying launcher children or the exact configured
`HTGame.exe`. Unrelated children are resumed without loading a DLL. A suspension
requested by the caller is retained; otherwise exactly the added count is
removed after the DLL's startup handshake. Arguments, environment, directory,
security attributes and original creation failures are forwarded. NTE's
`--extra` applies to the initial official launcher, not to its game command.

A fresh Local session mapping and exclusive per-PID activation/events coordinate
bridge and game roles. The controller must remain alive and the session active.
The bridge transfers a query/synchronize handle obtained from its own game
creation to the controller with `DuplicateHandle`; it never attaches by a
searched PID. A job tracks the newly created launcher tree and has kill-on-close
until UI initialization succeeds. Failure/cancellation cleans this owned tree;
success clears kill-on-close and deactivates further propagation. Existing NTE
launchers and games are rejected before starting this flow.

The controller allows five minutes to start the game from the official launcher.
Once the game DLL maps, its worker waits up to 120 seconds for `UnrealWindow`,
captures the loaded main module once, and retries only live reflection-cache
binding for 30 seconds. It pins its DLL and publishes originals before hooks.
Only the UI Ready result releases the tree. `BootReady` proves role initialization,
not UI installation. Opt-in logs are `nte-relay-<launcher-pid>.log` and
`nte-<game-pid>.log` beside the DLL.

Three aligned UFunction pointer slots and the navigation vtable slot use
compare/exchange with their verified previous values. Bool wrappers first call
the original exec to consume FFrame, then force the byte result to true. The
navigation wrapper replaces the requested input category with `Touch=2` and
calls the original NTE setter, preserving its locks and notification route on
the original calling thread. It does not call UE from the worker or write the
input enum field directly. A setter call must occur before this can change
the current navigation category.

The native C++ predicate also has direct callers that bypass reflection.
Discovery selects the installation method from its validated entry instructions,
not a region/version RVA table. The previously verified Global entry is a
four-byte, four-aligned `sub rsp,28h`; an aligned 32-bit compare/exchange publishes
`mov al,1; ret; nop`, flushes the cache and restores protection.

The supplied CN runtime showed `push rbx; sub rsp,20h`. This two-byte PUSH
cannot use that four-byte replacement. The saved-register path requires that
exact decoded frame form and an unwind-confirmed function start, then prepares
a disabled MinHook detour to a process-lifetime true-return callback. It rechecks
live entry bytes and enables the detour only after reflection/navigation slots
are installed. MinHook relocates whole instructions and adjusts suspended
callers' instruction pointers. Cache flush and restored page protection are
checked before reporting Ready. Other entry forms continue to fail with the
candidate RVA and bounded code bytes in diagnostics.

Install either native path last and never remove it live after publication.
The controller retains child cleanup on failure; the DLL and published callback
remain until process exit. A prepared trampoline is removed only if enabling
was never attempted. A failed enable may already have relocated a suspended
caller, so that trampoline remains allocated until owned process exit. Synthetic
fixtures clean up only after every test caller joins. No public disable/unload
or reinjection operation is supported.

Before native publication, failure attempts to restore only owned pointer
slots; a competing writer is preserved. After publication, protection/cache
failure is reported without live code restoration. The controller retains failure
cleanup until UI Ready. The DLL remains pinned
in launcher processes; after coordination ends its hook forwards calls without
propagating to further children.

## Native touch and mouse fallback

NTE 0.3.1 enables its touch corrections during ordinary startup, with or without
`--log`. No experimental environment variables or extra game arguments are
required. The UI Ready result includes validated input configuration and phone
paging installation. Failure of required discovery/installation uses the
coordinator's existing owned-child cleanup. Optional recording does not select
another touch implementation.

[input configuration](../games/nte/include/nte_input.hpp) uses the same immutable
main-module capture as UI discovery. It locates the UTF-16 configuration names,
validates the Slate constructor default, GetBool arguments and primary vtable,
and follows the unique constructor factory to singleton storage. The named
registration of `Slate.PreventDuplicateMouseEventsForTouchForWindows7` supplies
its writable CVar storage. Constructor prefix traversal accepts HDE-validated
LOCK bookkeeping; ordinary signature matching retains its strict prefix rules.
No client-version addresses or hashes are used.

A transient WH_CALLWNDPROC handshake performs configuration on the owned Unreal
window thread. It validates live singleton identity, boolean values and writable
storage before enabling duplicate-mouse filtering by compare/exchange. It never
writes `bTouchFallbackToMouse`; the tested value remains `1`. A previous combined
experiment that disabled fallback broke movement, skills and camera. The
command-line ini override did not change the live flag and is not a workaround.
The pinned handshake context survives a possible timeout.

[phone paging](../games/nte/include/nte_pager.hpp) follows the unique
`SHTPageScrollBox` construction helper to its derived vtable. Allocation size,
mouse-enter gating, touch-start fallback and inherited movement checks validate
the member/event offsets and eight explicit input-slot ABI assumptions. The
callbacks retain original arguments and FReply buffers, and compare/exchange
only verified previous slot values. Partial rollback preserves later writers.

The inherited pre-drag path checks capture across pointers, while established
drags query the actual event user/pointer. A pinned MinHook callback changes only
that verified pre-drag query during this pager's native touch movement, before
its drag flag is set. It requires both the exact native return site and a
thread-local widget scope. Other widgets/callers, mouse movement and established
drags use the original query. Native Slate still creates/releases capture and
constructs the reply; no replacement touch or mouse events are generated.

After native drag capture is requested, a missing pager start notification is
completed through the original weak-owner/delegate chain. The derived pressed
member changes only from zero to one. Below-threshold gestures and already
initialized presses are untouched. This matches the last tested configuration,
but it did **not** eliminate the first-swipe defect listed below.

### Known touch limitations

The 2026-11-28 CN live sessions used Moonlight + Sunshine with native touch.
The user confirmed combat, map dragging, settings clicks/dropdowns and settings
sliders working. Phone paging works after releasing and retrying, with these
remaining defects explicitly accepted for the 0.3.1 patch release:

- Each time the phone/ESC menu is opened, its first touch swipe fails in the recorded CN streaming sessions. Release
  the finger and swipe again; closing and reopening the menu resets the issue.
- The settings list scrolls farther than the finger moves while dragging. This
  concerns movement while still touching, not just release inertia. No speed
  adjustment is included in this release.

The user also confirmed both left- and right-button mouse dragging page the
phone; the wheel does not. The final initialization retest (game PID 5952,
login host PID 32872) left the same defects in place. Earlier capture-guard
records (game PID 111372) showed all 74 observed touch moves entering/continuing
native drag with handled replies. The filter-only session (PID 75356) separately
confirmed fallback `1` and duplicate filtering `0 -> 1` on the window thread.
These are CN streaming observations. Physical touch was unavailable; Global
input/pager/list discovery agrees with the supplied capture, but Global streamed
touch was not separately accepted. They are not a full multitouch or scene-
transition guarantee. The release promotion removes the log/environment gates;
its packaging pass uses fixtures and preserved samples, not another live session.

### Optional input diagnostics

With `--log`, [input tracing](../games/nte/include/nte_input_trace.hpp) installs
passive WH_GETMESSAGE and WH_CALLWNDPROC observers on the owned window thread.
`logs/nte-input-<pid>.csv` distinguishes queued/dispatch observations of the same
message and records touch/pointer phases, positions, extra-info, HWND and
focus/capture events. The game retains HTOUCHINPUT ownership. The observer never
closes those handles, edits/drops messages or creates replacement input.

A separate writer stops after 4096 records, 15 minutes or an explicit stop and
reports drops/termination. A destroyed startup window is rebound only to a
visible UnrealWindow in this process; HWND/thread identities are recorded. An
earlier header-only CSV provided no input evidence and must not be interpreted
as absence of Windows touch. No keyboard text or account frames are recorded;
no global Windows input hook or window-procedure replacement is installed.

Pager `S` records use `B`/`A` for before/after, the vtable byte slot as message,
pointer index as ID, and flag bytes for enabled/active/state/touch (state bits:
pressed, dragging, reply-handled). X/Y retain native delta float bits; the window
column contains the callback thread ID. The optional `SHTScrollBox` observer is
derived from its named/shared-base construction, inherited handlers and scalar
scroll getter. Its single move-slot wrapper forwards once and adds `L` input
phases and `G` geometry-scale/scroll-offset float bits. It does not alter speed.

### Native detour allocation

Before game main resumes, the owned game worker reserves one unused slot in the
pinned MinHook nearby-buffer pool. It retains that page through later hook
preparation and releases only its dummy slot after initialization; active
trampolines keep their allocations. This uses standard allocation and does not
change process protections, allocator reach or vendor source.

Predicate preparation failures include the target, captured Win32 last-error,
nearby free/aligned regions and available commit. The last-error value is
observational, not proof of cause. An early reserved slot accompanied successful
CN runs, but does not prove every intermittent `MH_ERROR_MEMORY_ALLOC` is fixed.

## Independent SDK login host

`--NTE --nte-region CN|Global --game <installed HTGame.exe>`
starts the bundled self-contained `nte-login-host/Nte.LoginHost.exe`, rather
than bare HTGame. `nte_login_host.hpp` resolves the exact installed Client
layout and region before elevation; ambiguous installations require a region.
The optional component is built with `scripts/build.ps1 -NteLoginHost` and
verified with `scripts/test.ps1 -NteLoginHost`.

The host is adapted from MIT-licensed Hi3Helper.Plugin.NTE commit
79863eeb2daa04d7558c54e2e6dc9a733102217e. See the vendored UPSTREAM.json,
ADAPTATIONS.md and upstream.patch for provenance and changes. Oddward was a
CN protocol reference; no GPL code was copied into this component.

The native controller creates the host suspended, assigns its owned job and
loads the Bridge role. Relay protocol 0x4e544503 adds the exact configured host
path outside the installation tree. Matching only the filename is insufficient.
The host creates its owned HTGame using .NET Process.Start; the existing
CreateProcessW hook preserves its environment, working directory, redirected
handles and original creation flags while early-loading the Game-role DLL.
Owned managed fixtures verify this actual .NET boundary, not only a C++ caller.
After UI Ready, the controller releases the job without terminating the host;
the SDK host must remain alive until game exit. Login/world acceptance is
independent of this UI Ready event.

The SDK component supplies named pipes, platform maps/windows/events, full
pre-start platform environment, regional bootstrap, bounded registration with
creation-owned peer PID validation, normal vendor authentication and session
callbacks. Global uses ArcGameMobileSdkType=2 and Saved_Global; CN uses selector
1 and its separate protocol. HTTP authentication uses the regional official
service. No fabricated accepted identity or skipped authentication is used.
User credentials are entered in the host or provider browser by the user.
Diagnostics contain fixed phase/failure identifiers, not credentials or frames.
Local accounts/preferences/DPAPI entropy/object names use TouchUILaunch rather
than upstream app storage. Region file switching and the upstream native plugin
loader are disabled; installed game files are never changed.

Global's existing 2.0.8 pair remains supported. The installed 2.0.9 pair has its
own exact launcher/bridge hashes, configuration key RVA 0xFAD728 and registration
version 2.0.9.0. Public app configuration was independently verified as data;
the runtime still rejects any mismatched pair or registration version. This is
an SDK compatibility table and is not used by the touch UI memory resolver.
CN retains upstream's separately verified 2.0.8/2.0.9 pairs. The CN backend is
resolved from NTELauncher rather than a potentially stale GameLauncher.config.

### Verification and live acceptance

Offline integration checks validate four registration contracts, wrong
PID/length/command rejection, mixed SDK pairs, argument round trips, Global
platform environment and account-store isolation. The installed Global 2.0.9
configuration verification sends no auth requests and runs no vendor code.
Native fixtures cover exact helper path classification, CN/Global selection,
ambiguous-region rejection, managed game creation, early mapping and owned
handle transfer. Synthetic entry-hook tests cover both atomic Global publication
and saved-register detours with concurrent callers, competing hook ownership,
pre-enable rollback and post-publication failure lifetime. The full optional
host suite also checks embedded resources without authenticating.

### Accepted live scenarios — 2026-11-27

| Region and launch mode | Evidence and accepted behavior | Scope still separate |
| --- | --- | --- |
| CN, independent SDK host, SDK 2.0.9 | Exact installed SDK pair matched; runtime logs show saved-register detour and UI Ready. After the fix, the user confirmed the requested login/world/basic-touch retest passed. | No separate control-by-control multitouch, streaming or scene-transition record. |
| Global, official relay, international client 1.5.5 | Verified DLL mapping through both launcher stages; world HUD screenshot and user confirmation of normal touch UI/operation. | Detailed multitouch, streaming and scene transitions. |
| Global, independent SDK host, SDK 2.0.9 | Static configuration/profile validation, real runtime initialization and user-confirmed successful login. | World/basic-touch acceptance for this specific host session was not separately itemized. |

The successful CN game worker was PID 94732, created by login host PID 70900.
Its log recorded native predicate RVA 0x8CDEC20, navigation slot RVA 0xE87F678,
`saved-register detour`, and successful installation. These RVAs describe this
observed run only; production does not select UI hooks by those addresses.
The first CN run was rejected because its entry saved RBX before allocating a
stack frame. The exact-entry adaptation fixed that failure; it did not change
login authentication or relax the remaining owner/reflection/navigation checks.

### Why the former bare direct launch failed

The previous bare direct experiment reached UI Ready but had no working login,
including a no-payload control with WPGameClientSDK_64.dll loaded. Arguments
and ArcGameSDKSharePath alone did not create a platform/session host. Server
switching and exit worked, but Enter Game only played a click sound and Logout
was absent. The bare path remains internal for owned loader fixtures;
the default `--NTE` route now uses the complete SDK host above.
`--nte-direct` remains a compatibility alias. `--nte-official` (or an explicit
`--nte-launcher` path) selects the official relay. Both launch modes are included in the same source tree.

## Current limits

A real owned-child attempt on the international 1.5.5 client on 2026-11-27
ended the diagnostic remote thread with status `0xDEADC0DE` and phase `0`.
The entry marker was never written, so `LoadLibraryW`, payload initialization,
and UI hooks were not reached. Queried dynamic-code/signature/image-load
restriction flags were zero. This is consistent with a process-specific
external-thread guard, but the responsible module has not been identified.
The current relay changes the loading stage to the owned official launcher and
the child-creation boundary. Its live result is recorded below.

This path uses existing NTE/Slate touch delivery. It adds no Windows event
bridge, multitouch emulation, CommonInput platform support override, asset
decryption, or complete HUD Blueprint adapter. The shared constant-false
stub and global `IsCloudGame` predicate are untouched. It does not force an
already-created player's input mappings to reload.

The DLL is mapped before game main, but UI installation still uses the Unreal
window as a startup timing heuristic. It may precede reflection
readiness or follow some UI/input initialization. A successful install can
therefore still leave the HUD/mapping unchanged. It must be tested before
claiming compatibility; an early-init hook or an explicit mapping-application
path may be needed based on observed behavior.

## Verified international 1.5.5 session — 2026-11-27

The built DLL was loaded through the actual installed launcher chain:

```text
TouchUILaunch.exe (elevated controller PID 96888)
  -> NTEGlobalLauncher.exe (PID 98820, bridge Ready)
     -> NTEGlobalGame.exe (PID 36088, verified mapping and bridge Ready)
        -> HTGame.exe (PID 118012, verified mapping and game worker)
```

The game worker recorded static discovery, reflection binding, predicate RVA
`0x8CE29B0`, navigation slot RVA `0xE878F18`, and successful hook installation.
The controller received UI Ready and exited; the game continued running.
The supplied screenshot shows the world HUD with the left virtual joystick and
right touch action controls. The user reported: “触控ui和操作没有问题。”
This is real-game UI/basic-operation evidence, distinct from the fixture checks.
No control-by-control multitouch or scene-transition checklist was collected.

Evidence from this build is retained under `dist/nte-mobile-ui/logs/` (ignored
runtime output): `launcher-96888.log`, `nte-relay-98820.log`,
`nte-relay-36088.log`, and `nte-118012.log`. The earlier late-load failure is
historical and does not describe the verified relay run. The user's screenshot
is not copied into the public repository or package.

## Manual acceptance

Build/package and run the default fixture suite first. For independent login,
include `-NteLoginHost` in the build and test commands. Exit NTE, its login host
and the official launchers before each deliberate live session. Choose a route:

- Official relay: `--NTE --nte-official --game <installed HTGame.exe> --log`, then click Start
  in the newly opened official launcher.
- Independent CN: `--NTE --nte-region CN --game <installed HTGame.exe> --log`.
- Independent Global: `--NTE --nte-region Global --game <installed HTGame.exe> --log`.

The user completes account authentication in the host/provider UI. Record:

- Launcher result and payload log, exact client build, and whether initialization
  completes before login/player creation. Capture the login UI result first.
- Mobile HUD after player creation, joystick visibility, movement, camera,
  skill presses, release/cancel behavior, and simultaneous contacts.
- Keyboard/mouse/gamepad interaction, focus loss, resize, reconnect, scene
  transitions, and any streamed-touch source separately from physical touch.
- Menu gestures: map pan through release and a second drag; horizontal phone
  page swipes; settings-list drag speed; dropdown tap to open, tap again to
  close, and option selection without holding; sliders following the finger
  and retaining their value on release. Check ordinary mouse input separately.
- Failure cleanup closes only the newly created launcher/game tree. A game already running
  must be rejected without injection or termination.

Do not infer visible behavior from a Ready event or a preserved sample match.
