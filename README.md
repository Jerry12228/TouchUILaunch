# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

This project enables touch UI support for several PC games.

## Supported games

- Genshin Impact
- Honkai: Star Rail
- Zenless Zone Zero
- Wuthering Waves

## Usage

1. Download the latest build from [Releases](https://github.com/Jerry12228/TouchUILaunch/releases), then place the packaged `.exe` and `.dll` in the same directory.
2. Run one of the commands below from that directory, replacing the sample path with the path to your game's executable.

Choose the target game with exactly one of `--GI`, `--SR`, `--ZZZ`, or `--WW`. Use `--game` to provide the executable path, `--log` to enable console logging, and `--extra <arguments>` to append one raw argument string to the game process. Quote the complete extra string when it contains spaces. The launcher passes it to every supported game; WW keeps its built-in cloud UI arguments before it.

Examples:

```powershell
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "path_to_root\StarRail.exe"
.\TouchUILaunch.exe --ZZZ --game "path_to_root\ZenlessZoneZero.exe"
.\TouchUILaunch.exe --WW --game "path_to_root\Client\Binaries\Win64\Client-Win64-Shipping.exe"
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe" --extra '-screen-width 1920 -screen-height 1080'
```

## TODO

- Support more games

One dream: to someday be capable of maintaining this project myself (an agriculture student's fantasy).

## Build

### Requirements

- Windows x64
- Visual Studio 2022 with the **Desktop development with C++** workload, including MASM x64
- CMake 3.24 or later

From the repository root, run:

```powershell
.\scripts\build.ps1
```

This script builds the minimum release artifacts: one launcher executable and one DLL.

## License

**If the relevant company wants to remove this warehouse, please submit the issue or contact me.**

This project is licensed under GNU General Public License version 3 only (`GPL-3.0-only`). See [LICENSE](LICENSE) for the complete terms.

The bundled HDE64 decoder retains its upstream [license and copyright notice](third_party/hde64/LICENSE.txt).

The retained legacy GI and current SR UI signatures and execution logic are adapted from [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker) under its [MIT License](third_party/gi_sr/LICENSE.txt).
