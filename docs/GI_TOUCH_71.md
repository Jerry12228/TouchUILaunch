# GI 7.1 evidence for release 0.2.0

This chapter records the **historical 0.2.0 implementation**. Its exact-file
gate and fixed address plan are superseded by [current discovery and joystick
setup](GI_TOUCH.md). Commands and behavior below describe 0.2.0.

This experimental implementation selects both the game's mobile UI layout and
TouchScreen input mode, using its existing Unity touch input path. It was derived from the supplied EXE and
the [independently recovered metadata](GI_METADATA_71.md), without using the
previous GI resolver, signatures, stubs, or external GI implementations.
The 0.2.0 launcher routes `--GI` to [gi_touch71.hpp](https://github.com/Jerry12228/TouchUILaunch/blob/0.2.0/games/gi/include/gi_touch71.hpp).
There is no fallback to the previous GI initializer.

## Supported input and behavior

Only the analyzed `GenshinImpact.exe` is accepted:

- File size: `444260776` bytes.
- SHA-256: `08a3086d5f3fe695f01dab61efa42e442006b18e5e475b2520df356f6a073b7d`.
- Preferred image base in the analysis: `0x140000000`. Runtime addresses use
  the newly created child's actual image base from its Windows x64 PEB.

The executable is opened read-only without write/delete sharing, verified,
and held across process creation. All ten loaded instruction windows must
match before the first write. Writes affect only the owned suspended child's
memory; instruction cache flushing, protection restoration, and readback must
succeed. Only then does the launcher resume and release the child. Any error
leaves the child owned by `game::Child`, which terminates it.

The startup default and UI setters select UI mode `Mobile=0`. The independent
input manager is also initialized to `TouchScreen=0`, and input transitions
retain that mode while preserving the game's input-map refresh calls.
Consequently, attempts through those setters to choose keyboard/controller
layouts remain in mobile layout for this launch. Existing comparison,
canvas-refresh, and notification code around the setters is preserved.
Restart the game normally to restore its normal layout selection.

Installed files, platform identity, graphics settings, frame rate, and system
input APIs are unchanged. This path requires no GI DLL or persistent injected
worker and does not alter protection software. Unsupported files are rejected
before game creation. The existing already-running-game rejection remains.

## Evidence chain

Numbers below refer to the recovered metadata's type/method IDs, not runtime
object indices. VAs are analysis addresses; the checked-in plan uses RVAs.

| Evidence | Finding |
| --- | --- |
| Type `15337`, `MoleMole.MonoRectTransformAdaptor` | Fields `_mobile`, `_pc`, `_ps`, `_mobileJoystick` identify layout branches. |
| Method `112496`, VA `0x14F2D4BC0` | Reads the singleton from static storage `+40112` and its mode field at `+864`; maps `0/1/2` to mobile/PC/PS layouts. |
| Type `34484`, `MBGAPCMOGML` | Enum names `Mobile`, `PC`, `PS`; branch behavior confirms `0/1/2`. |
| Type `38612`, `DAEMCCNOCPF` | Owns the mode field and setters below. Names are obfuscated. |
| Method `304937`, `NOJOIIHBMCD`, VA `0x14BA78C90` | Computes the default UI mode; mobile platforms select `0`, ordinary Windows selects `1`. |
| Method `305127`, `Init`, VA `0x14BA84DC0` | Calls the default selector at `0x14BA84DDC`, writes the result to `this+864`, then computes canvas parameters. |
| Method `304742`, `BBDNOCLIGKD`, VA `0x14BA61520` | Conditional mode setter; its argument is copied into `r8d`. |
| Method `304924`, `OCKNLKACCAL`, VA `0x14BA775B0` | Changes mode, refreshes canvas parameters, and optionally notifies UI objects. |
| Method `304868`, `AKHDGCPBDEP`, VA `0x14BA6FD00` | Leaf property setter writes `edx` to `this+864`. |

The [plan](https://github.com/Jerry12228/TouchUILaunch/blob/0.2.0/games/gi/include/gi_touch71_plan.hpp) changes 41 bytes total:

| Site RVA | Patch offset | Change |
| --- | --- | --- |
| `0x0BA84DC0` | `28` | Replace the five-byte default-selector call with `xor eax,eax; nop; nop; nop`. Init's prologue, unwind information, field store, and subsequent calls remain intact. |
| `0x0BA61520` | `4` | Replace `mov r8d,edx` with `xor r8d,r8d`. |
| `0x0BA775B0` | `24` | Apply the same argument clamp before comparison/refresh/dispatch. |
| `0x0BA6FD00` | `0` | Replace the seven-byte leaf body and first padding byte with `and dword ptr [rcx+864],0; ret`. The following function starts at `+16`. |

## Input-mode correction after the first live report

The user confirmed that the first 19-byte version displayed Mobile UI but
continued accepting keyboard/mouse input. Layout alone did not switch the
input device. This is a separate state and configuration path:

| Evidence | Finding |
| --- | --- |
| Type `80951`, `EKAEMFGFEMJ` | `TouchScreen=0`, `KeyboardWithTouchScreen=1`, `KeyboardWithMouse=2`, `Joypad=3`. |
| Type `31074`, `AADJAFKNJKO` | Input manager singleton at static storage `+40128`; current input mode at `this+372`, separate from UI mode at UI-manager `+864`. |
| Method `244072`, `IDMEAOEKCNL`, VA `0x150F48B90` | Ordinary Windows chooses `2`, writes input mode, then calls `0x150F424F0` to configure input. |
| Method `243790`, `CJGBIHJDHFI`, VA `0x150F42490` | Stores input mode and tail-calls the same refresh routine. |
| Method `244035`, `PLCNIMKNPBL`, VA `0x150F4A6A0` | Compares/stores the requested mode and refreshes controller bindings; its second argument is a refresh flag, retained unchanged. |
| Method `243876`, `FDDFPNEHHEH`, VA `0x150F41860` | Coordinates UI mode and input mode, including an **inlined UI store** and before/after notifications. Both mode arguments must agree. |
| VA `0x154033D20`, reached through `0x150F424F0` / `0x15402AE90` | Selects distinct controller-map groups for the input modes: touch `0` selects group `3`, keyboard/mouse `2` selects group `1`. |

The additional four sites are:

| Site RVA | Patch offset | Change |
| --- | --- | --- |
| `0x10F48B90` | `21` | Replace only this initializer's platform-query call with `eax=0`, selecting its existing TouchScreen branch and input refresh. The global platform query is unchanged. |
| `0x10F42490` | `4` | Clamp the input setter's copied argument `r8d` to zero; preserve the field store and tail-call to refresh. |
| `0x10F4A6A0` | `10` | Clamp `edi` before comparison, stores, and refresh; preserve the independent refresh flag. |
| `0x10F41860` | `33` | Clamp requested input (`r14d`) and UI (`ebx`) together before notifications and inlined updates. |

These changes target initialization and state transitions, not just the input
getter. This avoids reporting TouchScreen while leaving keyboard controller
maps installed. Runtime hotfix replacement remains outside this experiment.

The ordinary managed touch path also exists in this PC build:

- `MihoyoStandaloneInputModule` constructor (`0x14C373110`) enables its
  `_allowTouchInput` field at `+109`.
- Its touch-processing method (`0x14C371B30`) enumerates touch count, fetches
  touch data, and invokes the pointer press/move/drag/release handlers.
- `0x141445730` forwards count to `0x140A4EE40`, which reads the Unity touch
  collection. `0x1414456B0` fetches a bounds-checked 68-byte touch record.
- Touch support (`0x140A4EED0`) consults `GetSystemMetrics(94)` bit 6.

These facts justify trying the game's existing input path. They do not prove
that physical or streamed contacts reach it correctly. No synthetic Windows
touch bridge has been added.

## Joystick precision, camera sensitivity, and settings caption

The next live report described a small left joystick with insufficient control
precision, excessive right-side camera/aim speed at minimum sensitivity, and
a keyboard/mouse input caption. The source was a streaming client's native
touch mode; its name and host/client display metrics were not supplied.

The EXE establishes a common **physical touch scale**, separate from the
user's sensitivity indices:

| Consumer | Verified relationship |
| --- | --- |
| `UnityEngine.Screen.get_dpi`, method `17665`, VA `0x159C0D8E0` | Thunk to `0x14097C8F0`, confirming the engine DPI query. |
| `LLJPCNPPHIO.MCBNJLFNNAL`, method `462875`, VA `0x152CD7FE0` | Caches DPI at `0x145790A38`. Nonpositive DPI selects the game's own `360.0f` fallback (`0x43B40000`), then computes physical width/height from native resolution divided by DPI. |
| `MonoJoyStick_H4.Awake`, VA `0x152ED58E0` | Copies physical screen width from `0x145790A2C` to joystick `+172`. |
| `MonoJoyStick_H4.ResetGrpJoystick`, VA `0x152ED45D0` | Normal path scales the displayed joystick using physical radius `+44`, divided by physical screen width `+172`, multiplied by canvas width `+152`. |
| `MonoJoyStick_H4.PHOLIMHFNCE`, VA `0x152ED4AA0` | Uses `physicalRadius * renderWidth / physicalScreenWidth` as the drag/clamp radius. Thus changing the common scale changes useful travel as well as appearance. |
| `InputEasyTouch_H4.Start`, VA `0x147A614B0` | Copies the same physical width/height to `+32/+36`. |
| Its drag consumer, VA `0x147A61010` | On the ordinary non-cloud path, computes `(pixelDelta / renderSize) * physicalScreenSize`, then applies normal/aim and X/Y sensitivity obtained from the input manager. |

For equal native/render width, this reduces to joystick travel proportional
to `radius * DPI` and camera input proportional to `delta / DPI`. A host value
of 96 instead of 360 would give only 26.7% of the fallback joystick travel and
3.75 times its camera input, **before** applying the sensitivity slider. This
is a conditional calculation, not a measurement of the user's host.

The new site at RVA `0x12CD8013 + 19` replaces the two-byte `jb` with NOPs,
so this initializer runs its existing 360 DPI fallback even for positive host
DPI. It changes neither the engine's global DPI function nor resolution APIs.
The built-in class initialization, physical size computation, and subsequent
resolution scaling remain intact. This is a baseline for the user's streamed
touch setup, not a recovery of the touch client's actual DPI. Cloud-specific
overrides and other joystick component types are outside this confirmed path.

The settings investigation did **not** find evidence that the displayed
keyboard caption means the sensitivity sliders still edit mouse values:

- `EJEDLIBGCKE`, type `47647`, owns the `MonoSettingPage` context. Its sensitivity
  builder `0x149665250` reads input-manager `+372`. Mode 0 uses touch tables
  `0x148361BD0` (normal) and `0x1483616F0` (aim); mode 2 uses mouse tables.
- Camera sensitivity consumer `0x150F42DC0` makes the same selection. Touch
  indices are `+316/+432` for normal X/Y and `+440/+324` for aim X/Y. Mouse
  indices are distinct (`+356/+416` and `+392/+412`). Settings callbacks
  `0x149678CD0` and `0x149677340` call the normal X/Y setters; aim callbacks
  `0x149666790` and `0x14965F7F0` select the corresponding fields by mode.
- The dropdown caption builder `0x149653F20` instead chooses its first label
  by **platform**. On ordinary Windows it chooses literal 42474,
  `UI_SETTING_INPUT_TYPE_KEYBOARD`; mobile branches choose literal 42475,
  `UI_SETTING_INPUT_TYPE_TOUCHSCREEN`. Controller is literal 42476.
  Caption construction does not select a sensitivity table.

These keys were independently decrypted from metadata usage initializer
`0xE884` via `0x140523400` and literal decoder `0x140525D40`. The literal-slot
base from registration `0x142870B28 + 96` is `0x1459D9260`; indices 42474/42475
resolve exactly to the caption builder's `0x145A2C1B0/0x145A2C1B8` references.
Initializer `0xECA5` also resolves literal 42973, `UI_SETTING_INPUT_TYPE`.

The new caption site at RVA `0x09653F83` replaces that local platform-query
call with `mov eax,8`, selecting the existing touchscreen label and retaining
localization. Global platform identity stays unchanged. Slider ranges, saved
indices, normal/aim multipliers, acceleration, and input maps are not replaced
by invented sensitivity values. The earlier input-mode corrections still apply.

## Verification and limits

```powershell
./scripts/build.ps1
./scripts/test.ps1 -GiSample 'C:/Samples/GI/7.1/GenshinImpact.exe'
# Independent read-only sample check; this does not start the game:
./build/windows-x64-release/Release/GITouch71Tests.exe --sample 'C:/Samples/GI/7.1/GenshinImpact.exe'
# Manual run after closing any running copy:
./dist/TouchUILaunch.exe --GI --game 'C:/Games/GI/GenshinImpact.exe' --log
```

The default `GITouch71` test requires no game files. It covers validation before
mutation, unchanged surrounding instructions, conflicting sites, invalid and
overlapping ranges, write/readback failure propagation, executing the small
patch windows on owned memory, code-protection restoration, ASLR-aware image
lookup in an owned suspended fixture process, failure cleanup, and resume.
CPU fixtures additionally exercise both initial-selection windows, the input
setter argument, all four device modes, preservation of the refresh flag, and
coordinated UI/input arguments.
They also execute the original DPI comparison/fallback store with relocated
fixture data: positive host values initially survive, while the patched branch
uses 360 for 96/144/360/480 DPI, zero, negative, infinite, and NaN inputs.
An independent caption fixture executes the original conditional branches and
literal load, demonstrating Windows selects keyboard before the local override
and touchscreen after it. Fixture code is RX; its writable DPI storage is on
a separate RW page.
Its independently supplied continuations do not execute the game's refresh
implementation or certify gameplay.

Release build/package, all 8 default CTest cases, 124 CLI checks, the ZZZ
memory-boundary verifier, and the supplied EXE's read-only validation passed.
External ZZZ/SR samples were not supplied and those optional checks were skipped.
User feedback confirms Mobile layout and describes native streamed joystick
and camera interactions after the input-mode change. The scale and caption
corrections have passed automated checks, including all ten sample instruction
windows, but have not yet been verified in a real game session.
Scene changes, hotfix dispatch, physical multi-touch, and streaming delivery
remain unverified. Runtime hotfixes can replace the analyzed Init or setters;
this version neither overrides nor monitors hotfix replacement. A runtime
which rejects the memory changes must be treated as unsupported.

Do not generalize this exact-build experiment to all files labeled 7.1, CN
clients, later releases, or the old `UserAssembly.dll` layout. New builds need
fresh metadata/binary evidence and a reviewed plan.
