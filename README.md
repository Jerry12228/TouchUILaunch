# ZZZTouchUI 3.1

从最初可用的 3.1 实现整理的最小源码版本，仅保留启动器、触控桥、输入状态、自动提权和构建配置。

`profile.hpp` 中的 15 个游戏地址／字段偏移已全部留为 `0`，包括原先内联在 UI 就绪检查中的三个字段偏移。原版 3.1 文件哈希校验和 Unity Touch ABI 定义保留。需要自行填写与目标客户端匹配的值后重新编译；留空版本可编译，但除 `--help` 外会在提权或游戏操作前报错退出，DLL 同样不会安装钩子。

## 构建

需要 Windows x64、Visual Studio 2022 C++ 工具和 CMake 3.24+：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

输出为 `build/Release/ZZZTouchLauncher.exe` 和 `build/Release/ZZZTouchUI.dll`，使用时放在同一目录。填写偏移并构建后，通过 `--game` 显式指定游戏位置：

```powershell
.\build\Release\ZZZTouchLauncher.exe --game 'D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe'
```

保留本机／串流原生触控桥接和自动 UAC 提权，参数见 `--help`。默认自动附加或启动游戏并启用触控，不再提供指定 PID、状态查询或运行时启停参数。多个进程时使用 `--game` 按路径筛选，仍有多个匹配进程时需关闭多余实例。

默认不输出控制台日志，也不创建日志目录或文件。添加 `--log` 开启控制台和 DLL 文件日志，文件位于启动器目录下的 `logs/touch-<PID>.log`；该参数会在 UAC 提权后保留。再次运行启动器会按是否携带 `--log` 更新当前游戏进程的日志开关。`--probe` 的诊断输出同样需要 `--log`，`--help` 始终显示帮助；失败仍返回非零退出码。

此目录为独立 Git 仓库，分支为 `3.1`，首次提交只包含已移除偏移的最小源码，不继承原项目的历史、分析、测试、客户端文件或二进制。
