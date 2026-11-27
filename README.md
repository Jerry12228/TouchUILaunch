# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

This project enables touch UI support for several PC games.

## Supported games

- Genshin Impact
- Honkai: Star Rail
- Zenless Zone Zero
- Wuthering Waves
- Neverness to Everness (NTE)

## Usage

1. Download a package from [Releases](https://github.com/Jerry12228/TouchUILaunch/releases), choosing the edition that matches how you want to launch NTE. The two release packages use these names; replace `<version>` with the release version number without its leading `v`:

   | Package | Release archive name | Contents and when to choose it |
   | --- | --- | --- |
   | **Standard** | `TouchUILaunch-v<version>-win-x64-standard.zip` | Includes `TouchUILaunch.exe`, the shared `TouchUILaunch.dll`, and license notices. Choose this for Genshin Impact, Honkai: Star Rail, ZZZ, Wuthering Waves, or NTE through the official launcher using `--nte-official`. Does not include `nte-login-host/`. |
   | **NTE Login** | `TouchUILaunch-v<version>-win-x64-nte-login.zip` | Includes everything in Standard plus the complete `nte-login-host/` directory, its runtime/dependencies, and their license notices. Choose this for independent CN/Global NTE login without starting an official launcher process. It also supports every Standard use case. |

   **Selection:** if you want NTE's default independent login, choose **NTE Login**. If you only need the other games, or prefer NTE's official launcher, choose **Standard**. Standard users must select the official NTE route with `--nte-official` or an explicit `--nte-launcher` path; omitting the login host does not automatically switch launch modes. Both packages still require an installed game; NTE independent login also uses SDK/configuration files from that installation.

   Extract the complete package. Keep `TouchUILaunch.exe` and `TouchUILaunch.dll` together, and retain `nte-login-host/` when using NTE Login. For source builds, see [Build](#build).

2. Run one of the commands below from that directory, replacing the sample path with the path to your game's executable.

### Parameters

The parameters are divided into four categories: **game selection, game path, game-specific options, and other options**.

1. **Game selection (required):**
   - Genshin Impact: `--GI`
   - Honkai: Star Rail: `--SR`
   - Zenless Zone Zero: `--ZZZ`
   - Wuthering Waves: `--WW`
   - Neverness to Everness: `--NTE`

2. **Game path (required):**

   Use `--game` to specify the path to the game executable, for example:

   ```sh
   --game 'path/to/your/game/xxxx.exe'
   ```

3. **Game-specific options:**

   NTE:

   | Option | Required | Description | Value |
   | --- | --- | --- | --- |
   | `--nte-region <region>` | No | Select the NTE client region to launch | `CN` or `Global` |
   | `--nte-official` | No | Use the official launcher to start the game. Run this launcher first, then start the game from the official launcher while this launcher is waiting. | None |
   | `--nte-launcher <path-to-exe>` | No | Specify the official launcher path when it cannot be found automatically in official relay mode | String |

4. **Other options:**

   | Option | Required | Description | Value |
   | --- | --- | --- | --- |
   | `--log` | No | Enable console logging; otherwise, the launcher is silent | None |
   | `--extra '<arguments>'` | No | Append one raw string of launch arguments when starting the game | String |

Examples:

```powershell
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "path_to_root\StarRail.exe"
.\TouchUILaunch.exe --ZZZ --game "path_to_root\ZenlessZoneZero.exe"
.\TouchUILaunch.exe --WW --game "path_to_root\Client\Binaries\Win64\Client-Win64-Shipping.exe"
.\TouchUILaunch.exe --NTE --nte-region CN --game "path_to_cn_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --NTE --nte-official --game "path_to_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe" --extra '-screen-width 1920 -screen-height 1080'
```

### Notes

1. Exit the selected game before launching; for NTE, also exit the official launcher.
2. Logs are stored in the `logs/` directory beside the launcher.
3. If only one NTE region is installed, the client can be detected automatically; `--nte-region` is not required.
4. Official NTE mode cannot be combined with `--nte-region` or `--nte-direct`.

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

This script packages one launcher executable and one shared payload DLL.
ZZZ and NTE both use `TouchUILaunch.dll`; NTE direct login still needs the
separate `nte-login-host/` directory. The installer removes the obsolete
`TouchUILaunch.NTE.dll` when updating an existing package.
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
