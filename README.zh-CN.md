# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

本项目用于启用各种PC游戏的触控UI

## 支持游戏列表

- 原神（实验性：仅限已分析的 7.1 EXE）
- 崩坏·星穹铁道
- 绝区零
- 鸣潮

## 用法

1. 在 [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) 下载最新的构建，将包中的 `.exe` 和 `.dll` 置于同一目录下。
2. 在该目录运行以下命令，将示例路径替换为你的游戏可执行文件路径

使用 `--GI`、`--SR`、`--ZZZ`、`--WW` 中的一个指定目标游戏；使用`--game`指定游戏可执行文件路径；携带`--log`参数启用控制台日志输出。使用 `--extra <额外参数>` 可向所有游戏追加一段原样启动参数；包含空格时请将整段参数用引号包住。鸣潮会先保留内置的云游戏参数，再追加该参数。

GI 现使用独立分析得到的 [7.1 UI 实现](docs/GI_TOUCH_71.md)。启动前校验完整 EXE，本次运行固定使用移动布局和 TouchScreen 输入模式，并采用游戏内置的 360 DPI 触控尺度回退值，同步调整摇杆有效行程与镜头手势、修正设置页触屏标签。其他构建会被拒绝。已有移动布局和串流操作反馈；本轮尺度、标签修正及多指操作仍待实机验收。正常重启游戏即可恢复默认行为。

示例：

```powershell
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "path_to_root\StarRail.exe"
.\TouchUILaunch.exe --ZZZ --game "path_to_root\ZenlessZoneZero.exe"
.\TouchUILaunch.exe --WW --game "path_to_root\Client\Binaries\Win64\Client-Win64-Shipping.exe"
.\TouchUILaunch.exe --GI --game "path_to_root\GenshinImpact.exe" --extra '-screen-width 1920 -screen-height 1080'
```

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

此脚本将构建主要产物，包含一个启动器exe和一个dll。

## 协议

本项目采用 GNU 通用公共许可证第 3 版（仅限该版本，`GPL-3.0-only`）。完整条款见 [LICENSE](LICENSE)。

内置的 HDE64 解码器保留其上游[许可证和版权声明](third_party/hde64/LICENSE.txt)。

保留的旧 GI 代码与当前 SR UI 特征及执行逻辑移植自 [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker)，遵循其 [MIT 许可证](third_party/gi_sr/LICENSE.txt)。
