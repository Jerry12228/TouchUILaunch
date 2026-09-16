# GI / SR mobile UI source attribution

Adapted from the local `refs/Genshin_StarRail_fps_unlocker` snapshot of
[Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker),
copyright (c) 2024 NullName, under the [MIT license](LICENSE.txt).

The snapshot used for this adaptation has these SHA-256 file digests:

| Reference file | SHA-256 |
| --- | --- |
| `src/main.cpp` | `341dd1ad72b4df6b9314876ba0e7bdf01afafe43a888f99310ba2e9b4e517535` |
| `src/shellcode_header.h` | `3cde8557a617c86b36a2b29402d43ef91f3fdba42b5ae4d909edcee006d67931` |

`games/gi/include/mobile_resolver.hpp` adapts the three SR UI-state signatures
and the GI UI, input-object and initialization-call signatures. The separate
`games/gi/src/gi_stubs.asm` and `games/sr/src/sr_stubs.asm` files express
the UI execution sequence as standalone MASM blocks: SR writes UI type 2 every
500 ms; GI restores the initialization function, calls it, then calls the UI
and input setters with the reference arguments. The adaptation preserves
argument registers, return values and target page protection and checks for
unready objects. The original combined shellcode blob is not included.

FPS synchronization, frame-rate patches, power-save control, verification
hooks, custom DPI and the custom PE loader are not part of this adaptation.
The launcher uses standard Windows APIs for its owned child processes.

The build script includes this project's full license in the distributed
`THIRD-PARTY-NOTICES.txt`. Snapshot digests document provenance only; they are
not production game-version checks or runtime lookup keys.
