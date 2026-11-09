# GI mobile UI and joystick setup

The active GI path uses [gi_touch.hpp](../games/gi/include/gi_touch.hpp),
[signature discovery](../games/gi/include/gi_touch_plan.hpp), and
[joystick helper](../games/gi/include/gi_joystick_code.hpp). It keeps Mobile UI
and TouchScreen input, including the touchscreen settings caption, for the
owned game session. Restarting normally restores the game's default behavior.

The left joystick's diameter and usable travel match release 0.2.0.
Single-finger displacement, two-finger displacement, and pinch increments are
left to the game's original gesture producers. No additional speed multiplier
is installed. Absolute touch positions, start positions, finger IDs/counts,
phases, timing, and pinch distance are also preserved. The shared 360 DPI
baseline already present in 0.2.0 remains in place for joystick sizing.
The game's existing Unity touch input path remains responsible for delivery.

## Discovery and compatibility

The original 0.2.0 work was independently derived from metadata and the EXE;
its historical evidence remains in [GI_TOUCH_71.md](GI_TOUCH_71.md).
Current discovery follows the retained
[legacy resolver](../games/gi/include/mobile_resolver.hpp)'s approach of masked
instruction candidates plus decoded relationships.
It does not call the old GI initializer or install its stubs.

Production has no GI file hash allowlist, build-specific RVA table, or fixed
object field offsets. It scans executable, nonwritable PE sections and requires
one validated match for every one of eleven instruction windows. Encoded
addresses and object displacements are masked, then decoded. UI initializer
and setter fields must agree; input transition and setter fields must agree.
The initializer and refreshing setter must call the same layout calculator,
store its result in the same separate field, and that calculator must read the
UI-mode field. The leaf setter must lie inside the code interval spanned by
this calculator and the three discovered UI entry points. This uses GI's
observed per-type function clustering, not a fixed RVA, byte-distance limit,
or whichever candidate appears first. An identical same-field setter outside
that interval is ignored; multiple candidates inside it are rejected.
Gesture producers are neither discovery requirements nor patch targets.
Branch destinations used by the DPI and caption
changes, the 360.0f fallback, executable call targets, and data references are
also checked. Patch bounds and overlaps are validated before any write.

Offsets within matched instruction sequences describe instruction boundaries,
not locations in a particular EXE. Register use, opcodes, field relationships,
and relevant enum semantics must still match the analyzed code shape. A version
that moves these routines, relocates their operands, or moves supported fields
can be discovered automatically. The leaf check additionally requires the
observed UI method clustering: it cannot establish type ownership from the seven-byte leaf alone. A build
that interleaves unrelated same-field leaves in that interval or moves the UI
leaf outside it needs renewed analysis. A changed compiler/code shape or ambiguous
match is rejected; there is no fixed-address fallback. This is not a promise
of compatibility with every future build or runtime hotfix.

The supplied 7.1 and 7.2 EXEs have been checked as real files. All eleven
locations are compared with independent IDA oracles in
[test-only evidence](../games/gi/tests/gi_pattern_fixture.hpp). The default tests
relocate and reorder the UI component, move UI/input/layout/joystick fields,
exercise the loaded-image reader, reject inconsistent helper targets/fields
and in-component duplicate leaves, and ignore out-of-component same-field
leaves. Missing gesture-producer signatures still do not prevent discovery.
Other actual versions and live streaming behavior remain unverified.

### 7.2 logic review and ambiguity fix

Static analysis of the supplied 7.2 executable supports retaining all existing
eleven patches. Mobile UI and TouchScreen input remain independent states;
no new touch-enabling patch was identified. Preferred-base VAs below are
analysis evidence only; production does not consume this table.

| 7.2 evidence | Finding |
| --- | --- |
| UI initialization `0x14A211520`, default selector `0x14A211A90` | Stores UI mode at `this+880` (7.1: `+864`). The selector still chooses mobile `0`, PC `1`, and console `2`; forcing its result to zero remains appropriate. |
| Layout selection `0x14A1F6EC0`, calculator `0x14A205AB0` | Both read `+880`; modes `0/1/2` still select the mobile/PC/console canvas paths. Init and refresh both call the calculator and store its result at `+808` (7.1: `+832`). |
| UI setters `0x14A212B20`, `0x14A2193A0`, `0x14A207BF0` | Conditional, refreshing and leaf setters still write the same UI mode. Existing argument/result clamps retain their meaning. |
| Input initialization `0x14C29F420`, setter `0x14C2A8580`, transition `0x14C2A04C0` | Input field moves to `+344` (7.1: `+372`). Zero still follows the touch-only path; keyboard-with-touch, keyboard/mouse and joypad remain `1/2/3`. Existing refresh calls remain intact. |
| Coordinator `0x14C2A7620` | Still carries UI mode in `ebx`, input mode in `r14d`, contains an inlined UI store and invokes the input transition. Both arguments must still be clamped. |
| DPI `0x151D3B663`, caption `0x14D496DE3` | Same 360.0f fallback/store relationship and mobile caption branch topology. |
| Joystick copy `0x153282E89` | Still copies cached physical width into joystick `+172`; the existing factor of 1.0 and untouched gesture producers are retained. |

The original failure was reproducible before game creation:
`UI property setter: expected one validated match, got 2`.
Both `0x14A207BF0` and the unrelated `0x14FF516A0` have the identical body
`mov [rcx+880],edx; ret` and matching alignment padding. Field agreement alone
therefore no longer identifies the UI setter. The decoded shared calculator
at `0x14A205AB0` and the UI entry points bound the interval
`0x14A205AB0..0x14A2193A0`; only `0x14A207BF0` lies inside it. In 7.1 the
same relationships select `0x14BA6FD00` without version-specific branching.
The launcher continues to validate the entire discovered plan against loaded
instructions before any write.

Verification: the default nine-case CTest suite, CLI/package checks, and both
read-only GI sample oracles pass. This is static/sample and owned-fixture
evidence; it does not certify actual touchscreen delivery, camera gestures,
or live joystick behavior in 7.2.

## Joystick and gesture-path evidence

These addresses describe the analyzed 7.1 sample at preferred base
`0x140000000`; they are documentation evidence, not runtime lookup inputs.
Names come from the [recovered metadata](GI_METADATA_71.md), with instruction
and data flow checked in IDA. Obfuscated names are retained. Gesture rows record
analysis only; those producers and consumers are not patched by this path.

| Function / VA | Observed relationship |
| --- | --- |
| `MonoJoyStick_H4.Awake`, `0x152ED58E0` | Copies the cached physical screen width to the joystick's field `+172` at `0x152ED5A81`. |
| `MonoJoyStick_H4.ResetGrpJoystick`, `0x152ED45D0` | Display scale divides the configured radius by that width and multiplies by canvas width. |
| Joystick drag, `0x152ED4AA0` | Travel clamp uses `radius * renderWidth / physicalWidth`. Halving only the copied width doubles display diameter and useful travel together. |
| `BaseFinger.GetGesture`, `0x146E1CDE0` | Copies source delta `+88` into a gesture at `0x146E1CE33`, separately from position `+72` and start position `+96`. |
| `EasyTouch.CreateGesture2Finger`, `0x146FA2F10` | Writes fresh two-finger delta at `0x146FA3026` and deltaPinch at `0x146FA304C`; distance and timestamps are separate stores. |
| Camera drag, `MonoInputEasyTouch_H4` at `0x147A61010` | Reads gesture delta `+88`, then applies physical screen scale, normal/aim sensitivity and rotation scale. |
| World zoom, `MonoInputEasyTouch_H4` at `0x147A60E70` | Reads deltaPinch `+116` at `0x147A60F2D`, applies frame time and passes zoom input onward. It does not consume the DPI conversion used by camera drag. |
| `MonoLevelMapUI.InitTouchContenxt`, `0x14FEC92B0` | Initializes its `MonoExUITouch`; that component routes gestures to the current `MonoExUICamera` state. |
| `MonoExUICamera` state setup, `0x153B6A170` | Creates states including `AJMLOFMJAKH`; `GotoState` at `0x153B6CED0` installs the current state in field `+320`. |
| Map zoom state `AJMLOFMJAKH.LDEKACGBAAD` / `PLKOBFACHCM`, `0x153483700` / `0x153483800` | Instructions at `0x153483730` / `0x153483830` read the same gesture deltaPinch `+116`. They multiply by frame time and the camera's zoom ratio `+212` before invoking its zoom delegate `+456`. |

The existing 360 DPI fallback remains unchanged from 0.2.0. Changing DPI again
would couple camera drag and joystick size while leaving the observed pinch
paths unaffected. The joystick's copied width uses a factor of 1.0 to retain
the 0.2.0 size. Single-finger, two-finger and pinch producer instructions remain
byte-identical after setup, so their deltas are not scaled or rewritten.

The map pinch finding required disassembly: IDA's pseudocode for the state
callbacks omitted the floating-point data flow across the time getter and
indirect delegate. The `movss [r8+74h]` loads and following `mulss` instructions
establish this relationship; a callback's name alone does not.

## Process and code ownership

1. Open the selected EXE read-only without write/delete sharing. Discover its
   plan while holding the file across child creation.
2. Create a new, owned suspended process. Read its ASLR image base from the
   Windows x64 PEB. Snapshot executable sections and independently discover the
   loaded plan; its addresses and expected bytes must match the file plan.
3. Validate all eleven windows before writing. Allocate one nearby 4 KiB
   helper page in that child, prepare the joystick rel32 call, read back the helper
   code, and change its protection from RW to RX.
4. Apply and read back all patches, flush instruction caches, and restore
   original target protections. Any failure keeps the child owned and suspended
   for termination by `game::Child`. Unpublished helper allocations are freed.
5. Retain successful helper code until process exit, then resume and release
   the child. No worker, remote engine call, game-file edit, or GI DLL is used.

The joystick helper is leaf code: it leaves RSP, EFLAGS and nonvolatile
registers untouched and reproduces the original width store. Its scalar factor
is 1.0, with a range-checked rel32 call. Single-finger, two-finger and pinch
producers have no additional helpers or runtime hooks.

## Verification and manual acceptance

```powershell
./scripts/full.ps1 -DistDir dist/gi-pattern-touch-scale -GiSample 'C:/Samples/GI/7.1/GenshinImpact.exe'
# Discover another EXE without executing it or assuming the 7.1 oracle:
./build/windows-x64-release/Release/GITouchTests.exe --discover 'C:/Samples/GI/other/GenshinImpact.exe'
```

`-GiSample` / `--sample` retains the known 7.1 IDA oracle comparison;
`--discover` only checks signature/relationship acceptance, not compatibility.
Neither executes the supplied EXE. `GITouch` requires no game files and covers:

- File and simulated loaded discovery, relocated code and changed fields;
  missing/ambiguous signatures, field disagreement, branch changes, invalid
  targets, overlap, conflicts, write/readback failures, and unprepared hooks.
- Real x64 execution of UI/input substitutions, the DPI/caption branches, and
  the joystick helper through its installed rel32 call. Gesture producer
  windows must be untouched by the plan; independent harnesses execute their
  original stores with positive/negative/zero deltas and check unchanged values,
  source data, surrounding fields, timestamps and registers.
- Protection restoration, RX helpers, freed helper allocations and owned
  suspended-child cleanup. Only the test executable is launched by these tests.

Live acceptance must record client name, client/host resolution and game build.
Compare with 0.2.0 using the same scene and settings: joystick diameter and
edge travel; fixed-distance camera drag and aim; map pinch in/out; world zoom;
stationary fingers; taps, menus and simultaneous movement/camera interaction.
Check several frame rates and resolution/aspect-ratio combinations. This path
does not calibrate camera or zoom speed, or change upstream streaming input
transformations.
