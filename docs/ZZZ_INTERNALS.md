# ZZZ discovery and touch bridge

ZZZ has two cooperating implementations: the launcher creates the game and
loads the payload, while the payload discovers the loaded GameAssembly layout
and supplies Windows touches to Unity's existing input interface. See
[Game support](GAME_SUPPORT.md#zzz-launch-and-probe) for launcher behavior.

The main sources are [payload.cpp](../games/zzz/src/payload.cpp),
[profile_resolver.hpp](../games/zzz/include/profile_resolver.hpp),
[short_discovery.hpp](../games/zzz/include/short_discovery.hpp), and
[touch_state.hpp](../games/zzz/include/touch_state.hpp).

## Initialization and readiness

`DllMain` starts a worker and returns; it does not synchronously resolve or
install input hooks. The worker retains the launcher's logging event and
publishes the startup event before checking process identity and initializing
the bridge.

The worker then waits for GameAssembly, calls `discovery::resolve_module` once,
stores the resulting profile, selects a visible game window, subclasses it,
creates a timer, and waits for callable Unity input slots. The module, window,
and icall readiness loops each have their own roughly 120-second limit.
These limits are not one combined end-to-end deadline.

| Observable milestone | What it establishes | What remains |
| --- | --- | --- |
| Launcher verifies DLL module path | The intended payload was loaded | Worker setup may still fail |
| Startup event / `Payload started` | Worker retained logging configuration and published its event | Discovery, window selection, hooks, and UI may still be pending |
| `resolved all 15 fields from memory` | Semantic discovery returned a profile | Callable slots and UI objects need readiness checks |
| `ICall bridge installed` | Three input slots were replaced | UI provider and actual input still need validation |
| `READY: ... enabled=1` | Bridge was enabled at that instant | Message explicitly awaits UI provider and input |
| `Effective UI layout=1` | Effective-layout getter reported Mobile | Visible UI, multitouch, and streaming remain manual acceptance |

`active_profile` points to worker-owned resolved storage published before
window/hook use. Discovery is not periodically repeated. Window destruction,
UI faults, or hook conflicts disable behavior rather than starting a supported
reattach cycle. There is no public disable/unload CLI.

## Discovery pipeline

The runtime path is
`resolve_module -> capture_module -> discover`. The include adapter
[discover_profile.hpp](../games/zzz/include/discover_profile.hpp) selects the
current short/semantic discovery implementation.

1. **Retain and capture.** `resolve_module` pins GameAssembly for the process
   lifetime. `ModuleReader` validates address arithmetic, committed readable
   pages, allocation ownership, and complete reads using the current process.
2. **Parse an RVA view.** Shared [Image](../common/binary/include/pe_image.hpp)
   validates PE32+/x64 headers, section bounds and overlap, and owns snapshots
   of code sections. A memory view reads section RVAs; a disk view translates
   RVAs to raw offsets. It never executes inspected client instructions.
3. **Find Unity wrapper regions.** Unique masked input/screen/frame rules
   identify wrapper groups. RIP-relative decoding derives six icall slots,
   including the injected GetTouch tail-call shape.
4. **Prove layout selection.** A short core supplies candidates. Bounded
   instruction checks recover the effective-layout function and its override
   and default getters, requiring exactly one validated choice.
5. **Prove property ownership.** Accessor analysis derives class, pool, state,
   initialization byte, and property fields. Setter candidates must share the
   getter's provider and dispatch semantics; exactly one must target the
   override property.
6. **Prove consumers.** Direct calls or validated helper wrappers must connect
   PC/Mobile layout comparisons to the bounded count/GetTouch loop and the
   Mobile-to-TouchScreen device return. Mobile is `1`, PC is `2`, and the
   verified touch-device enum is `3` in these paths.
7. **Validate storage and evidence.** Eight storage slots must be distinct,
   aligned, and in suitable data sections. Return the 15 fields and bounded
   code evidence used by diagnostics and fixture extraction.

The [x64 reader](../common/binary/include/x64_reader.hpp) wraps HDE64 with
instruction/operand checks. Short patterns select candidates; decoded
relationships decide acceptance. A candidate rejected with `runtime_error`
may be skipped while considering others. `MemoryReadError` deliberately
derives directly from `std::exception`, so candidate-rejection handlers cannot
hide an incomplete or unreadable memory capture. Preserve that distinction.

### Property and interface dispatch

[property_reader.hpp](../games/zzz/include/property_reader.hpp) checks accessor
prologue/epilogue, readiness guards, property ownership, interface traversal,
and argument preservation. `same_owner` compares class/pool identity and
dispatch layout, not just a convenient byte prefix.

[dispatch_reader.hpp](../games/zzz/include/dispatch_reader.hpp) is a bounded
symbolic interpreter for the lookup fallback and direct interface paths. It
tracks values by role: object, setter input, method index, function, context,
and result words. It proves that both paths supply the same call roles even
when register choices, independent instruction order, or result-word order
vary. The getter uses interface slot `0`; the setter uses slot `1`.

Do not replace this with a fixed register/stack-word assumption. Conversely,
accepting arbitrary instruction sequences is not the goal: unknown dataflow,
clobbered live values, wrong slots, and inconsistent joins must still fail.
The [sample discovery tests](../games/zzz/tests/discovery_tests.cpp) contain
both valid permutations and rejection cases.

## Profile fields and address kinds

[profile.hpp](../games/zzz/include/profile.hpp) defines `profile::Build` and
the `profile::fields` enumeration. Every member has type `uintptr_t`, but the
members have different meanings. Add `assembly` only to module RVAs, not to
an object offset or an already dereferenced pointer.

| Field | Kind | Runtime use |
| --- | --- | --- |
| `touch_count_slot` | Module RVA of pointer storage | Original/replacement touch-count callback |
| `get_touch_slot` | Module RVA of pointer storage | Original/replacement `void(int, UnityTouch*)` callback |
| `touch_supported_slot` | Module RVA of pointer storage | Original/replacement touch-supported callback |
| `frame_count_slot` | Module RVA of pointer storage | Unity frame identifier getter |
| `screen_width_slot` | Module RVA of pointer storage | Unity width getter |
| `screen_height_slot` | Module RVA of pointer storage | Unity height getter |
| `ui_class_slot` | Module RVA of pointer storage | Pointer to the UI class |
| `static_reference_pool` | Module RVA of pointer storage | Pointer to the static reference pool |
| `ui_state_offset` | Offset within the pool | Pointer to the UI provider |
| `get_layout_override` | Module RVA of function | Read override with the current calling convention |
| `set_layout_override` | Module RVA of function | Set override through the game's notification path |
| `get_effective_layout` | Module RVA of function | Read effective override/default selection |
| `class_initialized_offset` | Offset within the class | Initialization byte |
| `override_property_offset` | Offset within the provider | Override property object |
| `default_property_offset` | Offset within the provider | Default property object |

In a disk view, `Image::slot` requires initially zero pointer storage, allowing
unbacked BSS bytes. In a loaded view it verifies readable storage without
requiring zero: the game may already have initialized those pointers. Actual
callback executability and provider readiness are checked later. Do not apply
disk-zero assumptions to initialized module memory.

## Memory discovery versus file diagnostics

The runtime resolver has no file/path/hash/loader dependency, no manual
version-address table, and no disk fallback. It pins an already-loaded module;
pinning is not loading a replacement from disk. Its code snapshot is owned,
while storage-slot validation reads live module memory.

[file_probe.hpp](../games/zzz/include/file_probe.hpp) is a separate read-only
diagnostic. It maps a disk DLL as data, hashes that mapping, and discovers
against the same bytes. `--probe` computes SHA-256 even when `--log` is absent;
logging determines whether the result is printed. The hash is not a lookup
key. Tests intentionally demonstrate discovery with an unknown hash and a
loaded mapping whose backing fixture file has been removed.

The payload uses file APIs for optional logging outside the resolver closure.
The [boundary checker](../games/zzz/tools/validate_memory_boundary.py) protects
the resolver closure, excludes file probing and test oracles from the release
payload, and inspects release imports/fingerprint strings. It is not dynamic
file-I/O tracing.

## Touch state and frame contract

`touch::UnityTouch` is an unboxed ABI representation with size `68` bytes and
`phase` at byte offset `36`, checked with `static_assert`. Its phases are
`Began=0`, `Moved=1`, `Stationary=2`, `Ended=3`, and `Canceled=4`. Keep the
injected callback signature and these layout checks aligned with the proven
Unity interface.

`touch::State` stores at most ten contacts, assigns stable finger IDs, and
retains terminal contacts until their terminal frame has been emitted. A quick
down/up still emits Began before Ended on a later frame; reusing a Windows ID
creates a new finger identity. Capacity includes contacts waiting for terminal
retirement. Additional downs increment `ignored_downs`.

Positions enter state as normalized client coordinates. A snapshot converts
them to Unity screen dimensions and flips Y: `(x * width, (1 - y) * height)`.
Movement deltas follow that coordinate system. `begin_frame` returns the same
snapshot for repeated calls with one frame ID, even if Windows messages arrive
between calls. Positive width/height are required to prepare a new snapshot;
payload frame delta is bounded before it reaches state conversion.

`hooked_count` and `hooked_touch` choose one source for a frame: the bridge
snapshot or the original Unity callbacks. They never append bridge contacts to
native counts or mix index spaces. Two empty bridge frames suppress native
echoes after contacts end, then native fallback resumes. An invalid bridge
index returns a canceled touch with finger ID `-1` when an output pointer is
provided. `hooked_supported` stays true while enabled or while terminal
contacts still need delivery, otherwise it calls the original callback.

See [touch-state tests](../games/zzz/tests/touch_state_tests.cpp) and
[production bridge harness](../games/zzz/tests/bridge_test_body.inc).

## Windows events and thread rules

The worker selects a visible, unowned window in its own process, favors
`UnityWndClass`, and requires a minimum client size. It subclasses the window
and installs a 250 ms timer; `maintain_ui` throttles UI work to 500 ms and
requires the window thread, installed hooks, and no active reentrant UI call.

- `WM_TOUCH` contributes source `1`; `WM_POINTERDOWN/UPDATE/UP` contributes
  source `2` only for `PT_TOUCH`. Both convert screen coordinates to client
  coordinates before normalization.
- Contact keys combine source and Windows ID. Switching sources requires a
  down event, no active keys, and at least 250 ms since the previous accepted
  source event. This suppresses duplicate event streams.
- Focus loss, cancel mode, and pointer capture changes cancel contacts.
  `WM_NCDESTROY` cancels, disables, clears the window, and kills the timer.
- The previous window procedure retains ownership of `HTOUCHINPUT`; the
  subclass forwards the message instead of closing that handle itself.
  Other messages are forwarded too, except its own handled timer message.

An exclusive SRW lock protects input state, source bookkeeping, and frame
snapshots. Shared readiness/counter variables use atomics, and file logging
uses a separate mutex. Do not invoke game UI setters while holding the input
lock: property notifications can synchronously reenter input handling.

`ready_ui_provider` checks class initialization, the provider pointer, and both
property objects. `maintain_ui` uses the real getter/setter rather than a raw
field write, preserving notification behavior. It remembers a changed provider
and its previous override. Restoration is conditional on the same provider
still holding Mobile (`1`), so an external writer's later value survives.
Touch-window registration is likewise removed only when this bridge added it.

## Hook installation and failure handling

The payload first checks that all six original input/frame/screen callbacks
are executable. It replaces GetTouch, touch-supported, then count slots using
compare/exchange. A failed step attempts to restore prior replacements using
their expected hook pointers. These are individually atomic replacements,
not a single atomic transaction across three slots.

Once installed, the worker checks for changed hook slots. A conflict disables
input, cancels contacts, logs the failure, and attempts to restore only slots
still owned by this bridge. It does not overwrite a competing pointer or
silently reinstall on top of it.

Native access violations/in-page errors during guarded UI calls set `ui_fault`,
disable input, and cancel contacts. Further UI calls are suppressed for that
process; arbitrary exceptions are not swallowed by the memory-fault filter.
Worker exceptions are logged and stop its initialization/loop. Neither path
implements a complete unload transaction. The DLL and installed callbacks must
remain resident; do not add `FreeLibrary` as error cleanup.

Restarting the game is the documented recovery for a UI fault, hook conflict,
or destroyed selected window. It is not a request to attach to an existing
process. Diagnose the cause with [Troubleshooting](TROUBLESHOOTING.md).

## Evidence and version adaptation

The checked-in [test data](../games/zzz/testdata/README.md) covers manual oracles
for 2.5, 2.6, 3.1, and 3.2, plus associated IDA instruction evidence. Generated
`test_profiles.hpp` is test-only. `patterns.json` records scan-rule provenance;
it is not a runtime version database.

For a client change, first identify whether failure occurs in capture,
candidate selection, semantic validation, callback readiness, or actual input.
For resolver changes, obtain independent evidence for expected fields, preserve
earlier samples, and add acceptance/rejection cases covering the changed
relationship. Keep mismatched or ambiguous candidates rejected. Do not repair
a failure by adding a production hash/address shortcut.

Schema, oracle enumeration, scan-rule extraction, and CTest version registration
are separate mechanisms. Review each when adding evidence; a new JSON file
alone does not update them. Regenerate only the affected artifacts using
[Development](DEVELOPMENT.md#generated-files), then run
[external sample regressions](TESTING.md#external-sample-tests) where available.
Report missing samples and manual gameplay checks explicitly.
