# TouchUILaunch native architecture

`launcher` owns command-line parsing, diagnostics, elevation and launch
serialization. It selects a game through the registered game descriptor and
does not contain per-game memory signatures.

`games/zzz` owns the resolver, read-only file probe and injected Windows touch
bridge. The runtime resolver reads only the loaded `GameAssembly.dll`; file
hashing is confined to the explicit diagnostic tool. `games/gi`, `games/sr`
and `games/ww` own their respective mobile UI behavior.

`common/windows` contains Windows resource and elevation helpers,
`common/process` contains owned-child and remote-process behavior, and
`common/binary` contains the PE and x64 readers. These directories must not
depend on a game module. `common/testing` contains only owned fixtures.

Use `cmake --preset windows-x64-release`, `cmake --build --preset
windows-x64-release`, and `ctest --preset windows-x64-release` for a standalone
build. `scripts/build.ps1` packages to `dist`; pass `-ZZZSampleRoot` only when
running external, read-only sample tests.
