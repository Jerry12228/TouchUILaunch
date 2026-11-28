# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

本项目用于启用各种PC游戏的触控UI

## 支持游戏列表

- 原神
- 崩坏·星穹铁道
- 绝区零
- 鸣潮
- 异环
- 明日方舟：终末地

## 用法

1. 在 [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) 下载适合自己启动方式的发布包。两个包采用以下固定命名，其中 `<version>` 替换为不含前缀 `v` 的发布版本号：

   | 发布包 | 压缩包文件名 | 内容与适用场景 |
   | --- | --- | --- |
   | **标准包（Standard）** | `TouchUILaunch-v<version>-win-x64-standard.zip` | 包含 `TouchUILaunch.exe`、共用的 `TouchUILaunch.dll` 和许可证。适合原神、星穹铁道、绝区零、鸣潮、终末地，以及通过 `--nte-official` 使用官方启动器启动异环的用户。不包含 `nte-login-host/`。 |
   | **异环独立登录包（NTE Login）** | `TouchUILaunch-v<version>-win-x64-nte-login.zip` | 包含标准包的全部内容，另附完整的 `nte-login-host/` 目录、运行时／依赖及其许可证。适合希望不启动官方启动器进程，直接登录国服／国际服异环的用户；也支持标准包的全部使用场景。 |

   **如何选择：**希望使用异环默认的独立登录方式，选择**异环独立登录包**；只玩其他游戏，或愿意通过异环官方启动器启动，选择**标准包**。标准包启动异环时必须选择官方中转方式（携带 `--nte-official`，或指定 `--nte-launcher` 路径），缺少登录宿主不会自动切换启动方式。两个包都需要已安装的游戏；异环独立登录仍会使用游戏安装目录内的 SDK 和配置文件。

   请完整解压，将 `TouchUILaunch.exe` 和 `TouchUILaunch.dll` 保持在同一目录下；使用独立登录包时保留完整的 `nte-login-host/` 目录。从源码构建请参阅[构建](#构建)。

2. 在该目录运行以下命令，将示例路径替换为你的游戏可执行文件路径

### 参数
本项目参数由**游戏选择、游戏路径、专用参数、其他参数**四部分构成

1. **游戏选择(必要)：**
- 原神：`--GI`
- 崩坏·星穹铁道：`--SR`
- 绝区零：`--ZZZ`
- 鸣潮：`--WW`
- 异环：`--NTE`
- 明日方舟：终末地：`--EF`

2. **游戏路径（必要）：**
使用`--game`指定游戏可执行文件路径，例如：
```sh
--game 'path/to/your/game/xxxx.exe'
```

3. **专用参数：**
- 异环

| 参数名 | 必要性 | 功能 | 值 |
|----------|---|---|---|
| `--nte-region <地区>` | 可选 | 指定所启动的《异环》客户端区域 | `CN`或`Global` |
| `--nte-official` | 可选 | 携带此参数将需要官方启动器启动，具体操为运行本启动器，启动器开始等待游戏启动时使用官方启动器启动游戏 | 无 |
| `--nte-launcher <exe路径>` | 可选 | 指定官方启动器路径，用于使用官方启动器模式且本启动器找不到官方启动器时 | 字符串 |

- 终末地

| 参数名 | 必要性 | 功能 | 值 |
|----------|---|---|---|
| `--ef-launcher <启动器Games.exe>` | 可选 | 指定终末地启动器路径 | 字符串 |

4. 其他参数

| 参数名 | 必要性 | 功能 | 值 |
|----------|---|---|---|
| `--log` | 可选 | 携带此参数启用控制台日志输出，否则静默 | 无 |
| `--extra '<额外参数>'` | 可选 | 启动时向游戏原样追加一段启动参数 | 字符串 |


示例：

```powershell
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "path_to_root\StarRail.exe"
.\TouchUILaunch.exe --ZZZ --game "path_to_root\ZenlessZoneZero.exe"
.\TouchUILaunch.exe --WW --game "path_to_root\Client\Binaries\Win64\Client-Win64-Shipping.exe"
.\TouchUILaunch.exe --EF --game "path_to_root\Endfield.exe" --log
.\TouchUILaunch.exe --NTE --nte-region CN --game "path_to_cn_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --NTE --nte-official --game "path_to_root\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe" --log
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe" --extra '-screen-width 1920 -screen-height 1080'
```

### 注意：
1. 请先退出目标游戏；异环还需退出官方启动器。
2. 日志位于启动器目录中的 `logs/`。异环携带 `--log` 时还会记录有数量与时长上限的输入诊断，供触控／拖动问题排查。
3. 异环只有一个已安装区域时可自动识别客户端，无须`--nte-region`指定。
4. 异环官方方式不能与 `--nte-region` 同时使用。
5. 终末地启动依赖官方启动器，默认自动查找，找不到时请使用`--ef-launcher <Games.exe>`指定

### 终末地启动与验证状态

1. 默认在 `%ProgramFiles%\GRYPHLINK` 下查找已安装的 `Games.exe`，优先选择有效的最新
   数字版本目录。安装在其他位置时，指定实际路径：

   ```powershell
   .\TouchUILaunch.exe --EF --game 'C:\Games\Endfield\Endfield.exe' --ef-launcher 'D:\Apps\GRYPHLINK\1.6.0\Games.exe' --log
   ```

   两处示例路径均需替换。指定文件必须是受信任且签名有效的 x64 GRYPHLINK `Games.exe`。
   TouchUILaunch 会自行创建官方父进程，无须再手动点击官方启动器中的“开始游戏”。
2. 出现 `Endfield touch UI state confirmed` 后，**保持 TouchUILaunch 运行直到游戏退出**。
   初始化最多等待 GameAssembly 120 秒，再等待 UI 状态 120 秒。确认成功后启动器继续
   运行属于正常行为。

曾反馈菜单／剧情点击偶发仅显示悬浮样式。本版加入针对该现象的独立 UI 指针策略，
改善效果仍待实机复测，详见[终末地验证](docs/EF_INTERNALS.md#manual-retest)。

### 异环已知问题

- 每次打开手机／ESC 菜单后，第一次触摸滑动仍会失败；松开手指再滑动可翻页，关闭重开后会再次出现。
- 设置列表在手指按住拖动时，滚动距离仍大于手指移动距离。


## TODO

- 支持更多游戏

一个梦想：有一天能有能力自己维护这个项目（一个农学生的幻想）


## 构建

### 需求

- Windows x64、
- Visual Studio 2022 (安装“使用 C++ 的桌面开发”工作负载（包含 MASM x64）)
- CMake >= 3.24

在仓库根目录执行：

```powershell
.\scripts\build.ps1
```

此脚本将打包一个启动器 EXE 和一个共用载荷 DLL。
绝区零、异环和终末地均使用 `TouchUILaunch.dll`；异环直接登录仍需单独的 `nte-login-host/` 目录。
更新现有分发目录时，安装步骤会移除旧的 `TouchUILaunch.NTE.dll`。
如需异环直接登录，请构建并验证可选的独立宿主：

```powershell
.\scripts\full.ps1 -NteLoginHost
```

此模式还需要 .NET 10 SDK，并会还原固定版本的 Avalonia 依赖。
分发时请保留完整 `dist/nte-login-host/` 目录和随包许可证。
详见 [开发说明](docs/DEVELOPMENT.md#optional-nte-login-host)。

## 协议

**如果相关公司希望移除本仓库，请提交issue或联系我。**

本项目采用 GNU 通用公共许可证第 3 版（仅限该版本，`GPL-3.0-only`）。完整条款见 [LICENSE](LICENSE)。

内置的 HDE64 解码器保留其上游[许可证和版权声明](third_party/hde64/LICENSE.txt)。

保留的旧 GI 代码与当前 SR UI 特征及执行逻辑移植自 [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker)，遵循其 [MIT 许可证](third_party/gi_sr/LICENSE.txt)。

异环与终末地的原生钩子使用 [MinHook](third_party/minhook/LICENSE.txt)。
独立登录宿主改编自 MIT 许可的 [Hi3Helper.Plugin.NTE](third_party/nte_login_host/LICENSE)，
固定来源及修改说明见 [ADAPTATIONS.md](third_party/nte_login_host/ADAPTATIONS.md)。
终末地的研究参考及致谢见[第三方声明](docs/THIRD-PARTY-NOTICES.txt)与
[终末地实现文档](docs/EF_INTERNALS.md#reference-and-sample-evidence)。
