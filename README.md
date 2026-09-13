# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

Enable touch controls and touch-oriented UIs in PC games.

## Supported Games

- Zenless Zone Zero

## Usage

1. Download the latest build from [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) and place the `.exe` and `.dll` files in the same directory.
2. Open PowerShell in that directory and run the following command, replacing the example path with the path to your game executable:

   ```powershell
   .\ZZZTouchLauncher.exe --game "D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe"
   ```

## Build

Building requires Windows x64, Visual Studio 2022 with the Desktop development with C++ workload, and CMake 3.24 or later. From the repository root, run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The build creates `build\Release\ZZZTouchLauncher.exe` and `build\Release\ZZZTouchUI.dll`. Keep both files in the same directory when running the launcher.

Game addresses and field offsets are resolved automatically from the loaded `GameAssembly.dll`; no manual offset configuration is required. The required headers and HDE64 decoder are included, so building does not require Python, IDA, game files, or generated test data.

When distributing the binaries, include the `LICENSE` and `THIRD-PARTY-NOTICES.txt` files copied to the build output directory.

## TODO

Add support for more games. For Genshin Impact and Honkai: Star Rail, use [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker).

## License

This project is licensed under the GNU General Public License v3.0 only (`GPL-3.0-only`). See [LICENSE](LICENSE) for the full terms.

The bundled HDE64 decoder retains its upstream [license and copyright notices](third_party/hde64/LICENSE.txt).
