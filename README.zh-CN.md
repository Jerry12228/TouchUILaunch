# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

本项目用于启用各种PC游戏的触控UI

## 支持游戏列表

- 绝区零

## 用法

1. 在 [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) 下载最新的构建，将包中的 `.exe` 和 `.dll` 置于同一目录下。
2. 在该目录打开 PowerShell，运行以下命令，将示例路径替换为你的游戏可执行文件路径：

   ```powershell
   .\ZZZTouchLauncher.exe --game "D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe"
   ```

## 构建

构建需要 Windows x64、安装“使用 C++ 的桌面开发”工作负载的 Visual Studio 2022，以及 CMake 3.24 或更高版本。在仓库根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

构建产物为 `build\Release\ZZZTouchLauncher.exe` 和 `build\Release\ZZZTouchUI.dll`。运行启动器时，请将两者放在同一目录。

游戏地址和字段偏移从已加载的 `GameAssembly.dll` 自动定位，无需手动配置偏移。所需头文件和 HDE64 解码器已包含在仓库中，构建不依赖 Python、IDA、游戏文件或生成的测试数据。

分发二进制时，请同时附带构建输出目录中的 `LICENSE` 和 `THIRD-PARTY-NOTICES.txt`。

## TODO

支持其他游戏。（原神和崩坏·星穹铁道请使用 [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker)）

## 开源协议

本项目采用 GNU 通用公共许可证第 3 版（仅限该版本，`GPL-3.0-only`）。完整条款见 [LICENSE](LICENSE)。

内置的 HDE64 解码器保留其上游[许可证和版权声明](third_party/hde64/LICENSE.txt)。
