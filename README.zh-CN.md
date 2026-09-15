# TouchUILaunch

[English](README.md) | [简体中文](README.zh-CN.md)

本项目用于启用各种PC游戏的触控UI

## 支持游戏列表

- 原神：移动 UI 和输入模式。
- 崩坏：星穹铁道：移动 UI。
- 绝区零：移动 UI 和 Windows 原生多指触控桥。
- 鸣潮：通过游戏自身的 Android 云游戏模式启用移动 UI。

GI／SR／WW 不使用 ZZZ 的多指触控桥。本项目不包含帧率解锁或帧率控制。

## 用法

1. 在 [Releases](https://github.com/Jerry12228/TouchUILaunch/releases) 下载最新的构建，将包中的 `.exe` 和 `.dll` 置于同一目录下。
2. 在该目录打开 PowerShell，运行以下命令，将示例路径替换为你的游戏可执行文件路径：

   ```powershell
   .\TouchUILaunch.exe --ZZZ --game "D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe"
   ```

必须且只能指定 `--GI`、`--SR`、`--ZZZ`、`--WW` 中的一个，不允许重复，参数顺序不限。GI／SR／WW 必须提供 `--game`；GI 也接受 `YuanShen.exe`，WW 接受 `Client-Win64-Shipping.exe`。路径应指向游戏本体，不能指向厂商启动器。

```powershell
.\TouchUILaunch.exe --GI --game "D:\Games\Genshin\GenshinImpact.exe"
.\TouchUILaunch.exe --SR --game "D:\Games\StarRail\StarRail.exe"
.\TouchUILaunch.exe --WW --game "D:\Games\Wuthering Waves\Client\Binaries\Win64\Client-Win64-Shipping.exe"
```

启动前请退出所选游戏。工具只创建新进程，不附加已有游戏，也不会自动结束已有游戏进程。需要管理员权限时显示 Windows UAC 提示；取消提权不会操作游戏。

添加 `--log` 后，控制台输出和错误会同步保存到启动器同目录的 `logs\launcher-<启动器PID>.log`，包含各初始化阶段。UAC 前后的两个启动器进程分别生成日志；独立提权控制台发生错误时还会弹窗显示原因，重定向输出时不会弹窗。ZZZ 另有 `logs\touch-<游戏PID>.log`。不带 `--log` 时不创建日志，错误保持静默并返回非零退出码。

使用 `--help` 查看参数。`--ZZZ --probe --game <exe> --log` 可进行无需提权、不启动游戏的只读文件诊断；`--probe-auto` 为其别名，两者均不支持 GI／SR／WW。

GI／SR 在新建进程的主线程保持挂起时准备初始化。GI 多处等价调用必须解析到相同函数和对象，实际目标冲突时拒绝启动。WW 正常启动并自动附加 `-CloudGame -CloudGamePlatform=Android`，不注入 DLL 或安装钩子。游戏更新可能使特征失效；钩子安装成功或进程恢复运行不等于游戏内 UI 与触控已经可用。工具不会修改游戏文件。

## 构建

构建需要 Windows x64、安装“使用 C++ 的桌面开发”工作负载（包含 MASM x64）的 Visual Studio 2022，以及 CMake 3.24 或更高版本。在仓库根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

构建产物为 `build\Release\TouchUILaunch.exe` 和 `build\Release\TouchUILaunch.dll`。运行启动器时，请将两者放在同一目录。

ZZZ 游戏地址和字段偏移从已加载的 `GameAssembly.dll` 自动定位，无需手动配置偏移。所需头文件和 HDE64 解码器已包含在仓库中，构建不依赖 Python、IDA、游戏文件、生成的测试数据或本仓库以外的文件。本最小源码包不包含测试文件或测试构建目标。

分发二进制时，请同时附带构建输出目录中的 `LICENSE` 和 `THIRD-PARTY-NOTICES.txt`。

## 开源协议

本项目采用 GNU 通用公共许可证第 3 版（仅限该版本，`GPL-3.0-only`）。完整条款见 [LICENSE](LICENSE)。

内置的 HDE64 解码器保留其上游[许可证和版权声明](third_party/hde64/LICENSE.txt)。

GI／SR UI 特征与执行逻辑移植自 [Genshin_StarRail_fps_unlocker](https://github.com/winTEuser/Genshin_StarRail_fps_unlocker)，版权为 (c) 2024 NullName，遵循其 [MIT 许可证](third_party/gi_sr/LICENSE.txt)。两份第三方许可证均合并到分发产物的 `THIRD-PARTY-NOTICES.txt`。
