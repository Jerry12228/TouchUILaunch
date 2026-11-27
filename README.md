# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

This project enables touch UI support for several PC games.

## Supported games

- Genshin Impact
- Honkai: Star Rail
- Zenless Zone Zero
- Wuthering Waves
- Neverness to Everness (NTE; CN direct login/touch accepted, Global relay HUD/controls and direct login verified)

## Usage

1. Download a build from [Releases](https://github.com/Jerry12228/TouchUILaunch/releases), or build from source. Keep the packaged executable and DLLs together; NTE direct login also needs the complete `nte-login-host/` folder.
2. Run one of the commands below from that directory, replacing the sample path with the path to your game's executable.

Choose the target game with exactly one of `--GI`, `--SR`, `--ZZZ`, `--WW`, or `--NTE`. Use `--game` to provide the executable path, `--log` to enable console logging, and `--extra <arguments>` to append one raw argument string. Quote the complete extra string when it contains spaces. NTE relay mode passes extra arguments to its official launcher; NTE direct mode and other games receive them directly. WW keeps its built-in cloud UI arguments before them.

Examples:

```powershell
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "path_to_root\StarRail.exe"
.\TouchUILaunch.exe --ZZZ --game "path_to_root\ZenlessZoneZero.exe"
.\TouchUILaunch.exe --WW --game "path_to_root\Client\Binaries\Win64\Client-Win64-Shipping.exe"
.\TouchUILaunch.exe --NTE --nte-region CN --game "path_to_cn_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --NTE --nte-region Global --game "path_to_global_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --NTE --nte-official --game "path_to_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe" --extra '-screen-width 1920 -screen-height 1080'
```

Exit the selected game before launching. For NTE, also exit the official launcher.
NTE requires the adjacent `TouchUILaunch.NTE.dll`; ZZZ uses `TouchUILaunch.dll`.
With `--nte-official`, NTE starts the official launcher with a process-creation relay. Click Start in
that launcher within five minutes. Its new game child loads our DLL before its
main thread resumes; the worker then waits for Unreal initialization and installs
mobile predicates and Touch navigation requests. Use `--nte-launcher <exe>` if
the official launcher cannot be located beside `Client`. Setup failure closes
this newly created launcher/game tree. Logs are under `logs/` beside the launcher.

By default, NTE uses the bundled `nte-login-host/Nte.LoginHost.exe`, which
provides the SDK platform context, login window and session for the game without
starting an official launcher process. Build with `-NteLoginHost` (requires .NET
10 SDK and the pinned Avalonia packages). The published host is self-contained.
Choose `--nte-region CN` or `--nte-region Global`; a single installed region can
be detected automatically. Keep the complete host folder beside the launcher.
The selected region's installed SDK/configuration files are still required;
updates use the official launcher. Only supported launcher/SDK pairs are accepted.
`--nte-direct` remains a compatibility alias for this default.
`--nte-launcher <exe>` also selects official relay mode with an explicit path.
Official mode cannot be combined with `--nte-region` or `--nte-direct`.

The native coordinator starts the login host suspended and installs the same
creation relay so the newly owned game loads the touch DLL before main. The
host supplies regional startup arguments, SDK objects and authenticated login;
it stays alive for the game's lifetime. Complete login in its window yourself.
Saved accounts, when enabled, use a separate TouchUILaunch store. UI Ready only
confirms touch hooks; login and world entry require separate live acceptance.
On 2026-11-27, the user confirmed the CN SDK 2.0.9 direct-login, world-entry
and basic-touch retest passed after adapting its saved-register native entry.
For Global, the international 1.5.5 official relay's mobile HUD/basic controls
were confirmed with a screenshot and user feedback, and independent-host login
was subsequently confirmed. Direct-host Global world/touch acceptance was not
separately itemized. Multitouch, streaming and scene transitions remain separate
acceptance cases. See [NTE implementation and testing](docs/NTE_INTERNALS.md).

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

This script packages one launcher executable and two game-specific DLLs.
For NTE direct login, build and verify the optional self-contained host:

```powershell
.\scripts\full.ps1 -NteLoginHost
```

This additionally requires the .NET 10 SDK and restores pinned Avalonia packages.
Keep `dist/nte-login-host/` and the packaged license notices when distributing.
See [development instructions](docs/DEVELOPMENT.md#optional-nte-login-host).

## License

**If the relevant company wants to remove this Repo, please submit the issue or contact me.**

This project is licensed under GNU General Public License version 3 only (`GPL-3.0-only`). See [LICENSE](LICENSE) for the complete terms.

The bundled HDE64 decoder retains its upstream [license and copyright notice](third_party/hde64/LICENSE.txt).

The retained legacy GI and current SR UI signatures and execution logic are adapted from [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker) under its [MIT License](third_party/gi_sr/LICENSE.txt).

The NTE process/predicate hooks use [MinHook](third_party/minhook/LICENSE.txt).
The independent NTE login host is adapted from MIT-licensed
[Hi3Helper.Plugin.NTE](third_party/nte_login_host/LICENSE); its pinned source
and adaptations are documented in [ADAPTATIONS.md](third_party/nte_login_host/ADAPTATIONS.md).
