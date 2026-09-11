# ZZZTouchUI 注入测试版

这是可编译、可运行的触控原型。它调用游戏自身的 Mobile UI 状态设置器，并将 Windows 原生触点接入游戏已存在的 Unity 触控读取接口。本机触摸屏（1）与串流原生触控（2）共用同一实现。能否在当前运行环境完成移动、视角和技能同时操作，仍以游戏实测为准。

当前版从已加载的 `GameAssembly.dll` 内存定位。2.5、2.6、3.1、3.2 共用一套短特征、指令解码、接口数据流及调用关系规则，生产程序没有固定地址表或版本／哈希选择。触控桥与自动提权流程沿用旧版。

注入 DLL 等待模块加载，保留模块至进程退出，读取内存 PE 头及可读代码节，在初始化时解析一次。正常运行不读取磁盘 GameAssembly，不计算文件哈希，不进行磁盘／内存代码比对。内存中的接口槽、类槽和引用池可以已经非零；实际就绪状态仍由接口安装与 UI 初始化流程检查。

匹配唯一、调用关系和边界检查全部通过时继续，否则记录错误并停止。运行时只需要启动器和同目录 DLL，不需要 IDA、Python、Capstone 或 metadata Dump。`--probe` 从文件使用同一匹配器解析并输出 SHA-256，仅用于只读诊断；未知哈希不影响解析资格。`--probe-auto` 是其兼容别名。

保留 `ZZZTouchLauncher.exe` 与 `ZZZTouchUI.dll` 在同一目录，更新测试版前先完全退出游戏。版本更新后无需修改配置即可尝试启动，但编译器、内联或混淆变化可能使自动定位失败；不能保证以后所有版本都免更新。

## 测试

在 `D:\WorkSpace\ZZZTouchUI\output\touch-ui` 打开 PowerShell。

```powershell
# 只读解析文件并输出 SHA-256，不启动或注入
.\ZZZTouchLauncher.exe --probe

# --probe 的兼容别名
.\ZZZTouchLauncher.exe --probe-auto

# 已启动游戏时附加；没有游戏进程时默认启动 Client/3.2 中的游戏
.\ZZZTouchLauncher.exe

# 可选：指定 PID，或指定另一处游戏（两者不要同时传入）
.\ZZZTouchLauncher.exe --pid 12345
.\ZZZTouchLauncher.exe --game 'D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe'
```

也可以直接双击启动器。普通权限下执行启动、附加、开关或状态命令时，会自动弹出 Windows UAC；确认后以管理员身份继续，并保留参数和工作目录。已经以管理员身份运行时直接继续。取消 UAC 会退出，返回 1223，不启动或操作游戏；提权未成功时不会循环弹窗。`--help`、`--probe`、`--probe-auto` 无需提权。

默认路径按项目目录布局寻找 `Client/3.2`；不存在时再寻找旧版目录。游戏安装在其他位置时使用 `--game`。为明确测试 3.2，可执行：

```powershell
.\ZZZTouchLauncher.exe --game 'D:\WorkSpace\ZZZTouchUI\Client\3.2\ZenlessZoneZero.exe'
```

程序使用普通 LoadLibrary 注入；若管理员权限下仍被客户端拒绝加载，应保留错误与日志用于判断。

日志为 `logs\touch-<PID>.log`。先出现模块基址 `Resolving loaded GameAssembly.dll at base=...`，随后是 `Resolver:` 阶段和全部 15 项解析结果。`DLL loaded` 只表示 DLL 已加载；`READY` 表示输入接口已接入；还应出现 `Effective UI layout=1`。进入可操作场景后依次验证：单指点击、摇杆持续移动、另一指拖动视角、移动期间按技能、抬起全部手指后停止动作、切出再切回。

所有版本都应出现 `resolved all 15 fields from memory`。读取失败会报告对应 RVA，范围不完整时不会用磁盘补齐。解析结果只用于当前进程；版本升级或替换测试版后需完全退出游戏再启动。

串流端必须向 Windows 传递原生触点。只将触屏映射为鼠标或手柄的模式没有独立多指信息，本原型不能从中还原多个触点。若使用 Moonlight/Sunshine，请在你的客户端中选择传递原生触控的模式；不同版本的选项名称可能不同。

```powershell
.\ZZZTouchLauncher.exe --status
.\ZZZTouchLauncher.exe --disable
.\ZZZTouchLauncher.exe --enable
```

`--status` 显示 DLL 是否加载、接口是否就绪及开关请求状态。`--disable` 在游戏窗口线程恢复由本工具设置的 UI 覆盖值，并发送触点取消；若其他代码已改写布局，不覆盖那次修改。DLL 保留在进程内，完全退出游戏即可卸载。工具自身不写入游戏文件或持久配置，UI 设置器引发的游戏内部通知仍需实测确认。

## 根据日志定位

| 现象 | 下一步 |
|---|---|
| 没有日志 | 检查启动器报错、DLL 是否同目录、进程和权限 |
| 有日志，没有 `READY` | 看内存解析、Unity 窗口或 icall 初始化错误 |
| 日志或只读探测报告 `Automatic profile:` | 保留完整错误；指令特征、候选数量或结构校验不满足，不会猜测地址 |
| `unreadable or incomplete module range` / `module read failed` | 代码或槽所在页面无法完整读取；保留 RVA、解析阶段和完整日志 |
| `READY`，但 UI 一直不是 1 | 保留日志；检查状态对象、游戏线程计时器和热更新分支 |
| `WM_TOUCH`、`WM_POINTER`、`downs` 都不增长 | Windows 游戏窗口没有收到原生触点；检查串流触控模式、焦点 |
| 消息增长，`downs` 不增长 | 事件可能是鼠标/笔、缺少按下阶段，或触点读取失败 |
| `downs` 增长，`count_calls` 不增长 | 当前场景没有经过已接入的 Unity 触控读取路径 |
| `bridge_frames` 增长、UI=1，仍无法操作 | 继续定位游戏的输入分发或热更新逻辑，不能据此宣布成功 |
| `another writer changed the icall table` | 初始化后的接口被其他代码改写，工具停止桥接；退出游戏后检查日志 |
| `native memory fault during UI call` | 当前进程停止 UI 调用；退出游戏并保留日志 |

日志记录接口状态和输入计数，不记录登录凭据。请连同“UI 是否变化、哪些动作可用、是否能同时多指”反馈测试结果。

## 已验证范围与构建

构建脚本：`D:\WorkSpace\ZZZTouchUI\scripts\build_touch.ps1`，需要 VS 2022 C++ x64、CMake、Python 3.11+ 和 Python Capstone 包（用于核对生成规则）。脚本使用保留的 2.5/2.6/3.1/3.2 样本运行测试，并复制两个二进制和第三方许可证；它不启动游戏。直接使用 CMake 编译不需要 Python 或 Capstone，HDE64 指令解码器静态编入 EXE/DLL。

`TouchState` 覆盖多指、坐标转换、按帧一致性、快速点击、ID 重用与容量；`TouchBridge31`、`TouchBridge32` 分别在独立进程的隐藏窗口及模拟接口表上测试生产桥接代码，包括原生接口回退、消息转发、重复事件源过滤、取消触点、主线程限制、UI 通知与恢复，以及两版不同的覆盖属性偏移。`profile-validation.json` 将每版 6 个接口槽、3 个 UI 函数的完整导出指令、状态字段及覆盖/默认值调用关系与原始 DLL 交叉核对。

`LauncherElevation` 用模拟 Shell 验证 UAC 取消、错误、循环防护与参数保留，并启动自有测试进程核对真实 CRT 参数解析和退出码传递。覆盖含空格、中文、引号和尾部反斜杠的参数，不弹出真实 UAC。

`ProfileDiscovery25`、`ProfileDiscovery26`、`ProfileDiscovery31`、`ProfileDiscovery32` 分别比较完整文件、模拟加载内存与人工基准的全部 15 项。每版保留 27 类错误候选、重复匹配、ABI 和关系拒绝测试、四类等价指令变形及 setter 移址测试；新增双加载基址、非零槽、不可读槽、删除文件后从内存定位和只修改内存代码的拒绝测试。

`MemoryReader` 覆盖文件偏移与 RVA 分离、跨页面指令、BSS、代码快照独立性，以及 17 类非法内存／PE 头拒绝条件。客户端模拟加载使用自有非执行内存，不调用 `LoadLibrary`，不执行客户端 DLL。`validate_memory_boundary.py` 检查内存解析器的源码依赖、正常路径的单次调用、诊断分支和发布导入；它不是操作系统级文件访问跟踪。

这些检查不等价于真实触摸屏、串流链路或战斗场景验收。此版不改变云平台、服务器或账号设置；没有驱动组件或反作弊绕过功能。3.1 研究依据见 `analysis/touch/injection.md`，3.2 地址、字段变化和反编译证据见 `analysis/versions/3.2/touch-adaptation.md`。

人工基准保留在 `profiles/*.json` 与 `analysis/versions/2.5`、`analysis/versions/2.6`。`generate_touch_profiles.py` 分别生成不含地址值的生产字段结构和仅测试使用的 `test_profiles.hpp`；基准与哈希不编入发布程序。原可用版本保存在 `baseline-3.1` 标签和 `releases/baseline-3.1`，其后的固定地址与 v1/v2/v3 发布同样保留。同一进程无法卸载后安全更新 DLL，回退测试前须完全退出游戏。

短布局特征和 Unity 规则仍来自保留的样本；接口派发以数据来源识别成员角色。当前设计与验证见 `analysis/runtime-resolution/memory-resolution.md`，历史数据流设计见 `dataflow-dispatch.md`。统一读取路径不保证任意混淆、内联或指令重写都能识别，也不使用固定地址兜底。开发分支为 `codex/memory-resolution`，发布归档为 `releases/runtime-resolution-v4`；分发须保留 `THIRD-PARTY-NOTICES.txt`。
