# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

本项目用于启用各种PC游戏的触控UI

## 支持游戏列表

- 原神
- 崩坏·星穹铁道
- 绝区零
- 鸣潮
- 异环（NTE；国服直接登录与触控测试通过，国际服中转触控及直接登录已验证）

## 用法

1. 在 [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) 下载构建，或从源码构建。将包中的启动器和 DLL 放在同一目录下；异环直接登录还需要完整的 `nte-login-host/` 目录。
2. 在该目录运行以下命令，将示例路径替换为你的游戏可执行文件路径

使用 `--GI`、`--SR`、`--ZZZ`、`--WW`、`--NTE` 中的一个指定目标游戏；使用`--game`指定游戏可执行文件路径；携带`--log`参数启用控制台日志输出。使用 `--extra <额外参数>` 可追加一段原样启动参数；包含空格时请将整段参数用引号包住。异环中转模式将该参数传给官方启动器；异环直接启动模式和其余游戏直接接收该参数；鸣潮会先保留内置的云游戏参数。

示例：

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

请先退出目标游戏；异环还需退出官方启动器。NTE 使用启动器旁的
`TouchUILaunch.NTE.dll`，ZZZ 使用 `TouchUILaunch.dll`。添加 `--nte-official` 时，NTE 挂起创建已安装的官方启动器并加载进程创建桥接代码，请在五分钟内点击其中的开始游戏。新建游戏的主线程恢复前
即加载我们的 DLL，之后等待 Unreal 初始化，安装移动端判定并通过原导航设置函数
请求 Touch 输入。如果无法自动找到官方启动器，可补充 `--nte-launcher <exe路径>`。
初始化失败会关闭本次新建的启动器及其游戏进程树。
日志位于启动器旁的 `logs/`。2026-11-27 已通过本构建的日志、用户截图与操作反馈验证国际服 1.5.5 的移动 HUD
及基本操作；多指、串流和场景切换仍按独立用例验收，详见 [NTE 实现与测试说明](docs/NTE_INTERNALS.md)。

NTE 默认由随包的 `nte-login-host/Nte.LoginHost.exe` 提供独立的
SDK 平台环境、登录窗口和会话，无需运行官方启动器进程。构建时加 `-NteLoginHost`，
需要 .NET 10 SDK 和固定版本的 Avalonia 依赖；发布包自带运行时。请保留完整宿主目录。
使用 `--nte-region CN` 选择国服，`--nte-region Global` 选择国际服；只有一个已安装区域时可自动识别。
仍需保留对应区域的官方 SDK、配置及资源文件，客户端更新继续使用官方启动器。
程序只接受已适配的启动器／SDK 组合。
`--nte-direct` 保留为默认独立登录方式的兼容参数；
`--nte-launcher <exe路径>` 也会选择官方中转方式并指定启动器路径。
官方方式不能与 `--nte-region` 或 `--nte-direct` 同时使用。

原生启动器挂起创建独立宿主并安装进程创建中转，新游戏主线程运行前会加载触控 DLL。
宿主负责对应区域的启动参数、SDK 对象和真实登录，在游戏运行期间持续保留会话。
请在登录窗口自行完成账户认证；选择记住账户时，使用独立的 TouchUILaunch 存储目录。
“UI Ready”仅表示触控钩子初始化成功，登录、进入世界仍需单独进行实机验收。
2026-11-27，国服 SDK 2.0.9 在适配保存寄存器的原生函数入口后，用户确认直接登录、
进入世界和基本触控的本轮测试通过。国际服 1.5.5 已通过官方中转方式验证移动 HUD 与基本操作，
独立宿主登录也已得到用户确认；国际服独立宿主的入世界与触控未另行逐项验收。
多指、串流和场景切换仍需独立测试。详见 [NTE 实现与测试说明](docs/NTE_INTERNALS.md)。

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

此脚本将打包一个启动器 EXE 和两个游戏专用 DLL。
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

异环进程与原生判定钩子使用 [MinHook](third_party/minhook/LICENSE.txt)。
独立登录宿主改编自 MIT 许可的 [Hi3Helper.Plugin.NTE](third_party/nte_login_host/LICENSE)，
固定来源及修改说明见 [ADAPTATIONS.md](third_party/nte_login_host/ADAPTATIONS.md)。
