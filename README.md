# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

Enable touch controls and touch-oriented UIs in PC games.

## Supported Games

- Genshin Impact: mobile UI and input mode.
- Honkai: Star Rail: mobile UI.
- Zenless Zone Zero: mobile UI and native Windows multi-touch bridge.

GI/SR do not use the ZZZ multi-touch bridge. No frame-rate unlocking or frame-rate control is included.

## Usage

1. Download the latest build from [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) and place the `.exe` and `.dll` files in the same directory.
2. Open PowerShell in that directory and run the following command, replacing the example path with the path to your game executable:

   ```powershell
   .\ZZZTouchLauncher.exe --ZZZ --game "D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe"
   ```

Select exactly one of `--GI`, `--SR`, `--ZZZ`. Selectors are required and cannot be repeated; argument order does not matter. GI/SR require `--game`. GI also accepts `YuanShen.exe`. Use the actual game executable, not HoYoPlay's launcher.

```powershell
.\ZZZTouchLauncher.exe --GI --game "D:\Games\Genshin\GenshinImpact.exe"
.\ZZZTouchLauncher.exe --SR --game "D:\Games\StarRail\StarRail.exe"
```

Exit the selected game first. The tool only starts new processes; it does not attach to or automatically close running games. Windows requests administrator approval when needed; canceling UAC performs no game action.

Add `--log` to save console output and errors to `logs\launcher-<launcher-PID>.log`, including initialization stages. The launcher before UAC and its elevated child have separate logs. An error in an isolated elevated console also displays a message box; redirected output does not. ZZZ additionally writes `logs\touch-<game-PID>.log`. Without `--log`, no logs are created and errors return a nonzero exit code silently.

`--help` shows available options. `--ZZZ --probe --game <exe> --log` performs read-only file diagnosis without elevation or game launch. `--probe-auto` is its alias; neither probe supports GI/SR.

GI/SR prepare initialization while the new main thread is suspended. Equivalent GI call sites must resolve to the same functions and objects; conflicting targets are rejected. Game updates may invalidate signatures. An installed hook or resumed process alone does not prove in-game UI or input works. The tool does not modify game files.

## Build

Building requires Windows x64, Visual Studio 2022 with the Desktop development with C++ workload (including MASM x64), and CMake 3.24 or later. From the repository root, run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The build creates `build\Release\ZZZTouchLauncher.exe` and `build\Release\ZZZTouchUI.dll`. Keep both files in the same directory when running the launcher.

ZZZ game addresses and field offsets are resolved automatically from the loaded `GameAssembly.dll`; no manual offset configuration is required. The required headers and HDE64 decoder are included, so building does not require Python, IDA, game files, generated test data, or files outside this repository. This minimal source package contains no tests or test targets.

When distributing the binaries, include the `LICENSE` and `THIRD-PARTY-NOTICES.txt` files copied to the build output directory.

## License

This project is licensed under the GNU General Public License v3.0 only (`GPL-3.0-only`). See [LICENSE](LICENSE) for the full terms.

The bundled HDE64 decoder retains its upstream [license and copyright notices](third_party/hde64/LICENSE.txt).

GI/SR UI signatures and execution logic are adapted from [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker), copyright (c) 2024 NullName, under its [MIT license](third_party/gi_sr/LICENSE.txt). Both third-party licenses are included in `THIRD-PARTY-NOTICES.txt`.
