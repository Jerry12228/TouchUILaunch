# GI／SR／ZZZ／WW 移动 UI 测试版

启动时必须指定 `--GI`（原神）、`--SR`（星穹铁道）、`--ZZZ`（绝区零）或 `--WW`（鸣潮）中的一个，参数顺序不限，不允许重复或组合。四款游戏均只支持通过本工具启动；所选游戏已运行时会报错，需先退出。运行中附加不符合初始化时序，已删除该无效分支。其他游戏类型的进程不影响所选游戏启动。

GI／SR 按 `refs/Genshin_StarRail_fps_unlocker` 的移动 UI 逻辑移植，不包含帧率解锁、帧率控制或节能调帧。GI／SR 必须通过 `--game` 指定现有 EXE；GI 接受 `YuanShen.exe` 和 `GenshinImpact.exe`，SR 接受 `StarRail.exe`。路径与所选游戏必须一致。

```powershell
.\ZZZTouchLauncher.exe --GI --game 'D:\Games\Genshin\YuanShen.exe' --log
.\ZZZTouchLauncher.exe --GI --game 'D:\Games\Genshin\GenshinImpact.exe'
.\ZZZTouchLauncher.exe --SR --game 'D:\Games\StarRail\StarRail.exe' --log
.\ZZZTouchLauncher.exe --ZZZ --game 'D:\Games\ZZZ\ZenlessZoneZero.exe'
.\ZZZTouchLauncher.exe --WW --game 'D:\Games\Wuthering Waves\Client\Binaries\Win64\Client-Win64-Shipping.exe'
```

WW 必须通过 `--game` 指定鸣潮安装目录下的 `Client\Binaries\Win64\Client-Win64-Shipping.exe`。启动器以 EXE 所在目录为工作目录，直接启动游戏并自动添加 `-CloudGame -CloudGamePlatform=Android`，无需手工传入这两个游戏参数。WW 与其他游戏共用自动提权流程：普通权限下先请求 UAC，确认后启动；已经是管理员时直接继续。WW 不挂起主线程、不安装钩子，也不依赖 `ZZZTouchUI.dll`；触控 UI 由游戏自身的 Android 云游戏启动模式启用。`--log` 可查看启动路径、参数和进程 ID。

GI／SR 以挂起主线程的方式创建进程，在恢复前完成模块加载和 UI 逻辑安装。GI 从主 EXE 的 `il2cpp` 节定位；旧布局使用 `<EXE名称>_Data\Native\UserAssembly.dll`。一次性钩子先恢复原代码及页面保护，调用原初始化函数，再调用 UI／输入设置器。SR 从 `GameAssembly.dll` 定位 UI 状态，独立任务每 500 毫秒将其设为 `2`。解析出的目标必须唯一且范围有效；GI 同一组函数／对象在多处出现时，逐一核对并归并相同目标，实际目标冲突仍会拒绝；准备失败只清理本次创建的进程，不结束已有游戏进程。

GI／SR UI 执行代码编入启动器，无需加载 `ZZZTouchUI.dll`；其触控能力以参考项目为范围，没有接入 ZZZ 的 Windows 原生多指触控桥。`--log` 输出模块地址、解析结果及安装状态到控制台，并将启动器标准输出和错误同步写入同目录 `logs\launcher-<启动器PID>.log`。UAC 前后两个启动器进程各有日志，逐步记录创建进程、加载器初始化、模块读取、特征解析和钩子安装。独立的提权控制台发生错误时会弹窗显示原因及日志路径，重定向输出时不会弹窗。不创建 GI／SR DLL 日志。“installed”仅表示初始化逻辑已安装，不代表游戏内移动 UI 已实测成功。兼容范围限于参考项目覆盖的特征；已对本机 GI 7.0 EXE 和仓库 SR 样本进行只读解析；用户反馈 SR 已正常启动。GI 7.0 的多处等价调用特征现已修复，完整游戏内交互仍需实机确认。

`--probe` 和 `--probe-auto` 仅适用于 `--ZZZ`；与 `--GI`／`--SR`／`--WW` 组合会报错，且不会请求 UAC 或启动游戏。无参数双击不会启动游戏，可建立带游戏选择参数的快捷方式。以下为 ZZZ 触控桥与诊断说明。

## ZZZ 触控桥

这是可编译、可运行的触控原型。它调用游戏自身的 Mobile UI 状态设置器，并将 Windows 原生触点接入游戏已存在的 Unity 触控读取接口。本机触摸屏（1）与串流原生触控（2）共用同一实现。能否在当前运行环境完成移动、视角和技能同时操作，仍以游戏实测为准。

当前版从已加载的 `GameAssembly.dll` 内存定位。2.5、2.6、3.1、3.2 共用一套短特征、指令解码、接口数据流及调用关系规则，生产程序没有固定地址表或版本／哈希选择。触控桥与自动提权流程沿用旧版。

注入 DLL 等待模块加载，保留模块至进程退出，读取内存 PE 头及可读代码节，在初始化时解析一次。正常运行不读取磁盘 GameAssembly，不计算文件哈希，不进行磁盘／内存代码比对。内存中的接口槽、类槽和引用池可以已经非零；实际就绪状态仍由接口安装与 UI 初始化流程检查。

匹配唯一、调用关系和边界检查全部通过时继续，否则停止（携带 `--log` 时记录错误）。运行时只需要启动器和同目录 DLL，不需要 IDA、Python、Capstone 或 metadata Dump。`--probe` 从文件使用同一匹配器解析，携带 `--log` 时输出 SHA-256，仅用于只读诊断；未知哈希不影响解析资格。`--probe-auto` 是其兼容别名。

保留 `ZZZTouchLauncher.exe` 与 `ZZZTouchUI.dll` 在同一目录，更新测试版前先完全退出游戏。版本更新后无需修改配置即可尝试启动，但编译器、内联或混淆变化可能使自动定位失败；不能保证以后所有版本都免更新。

## 测试

在 `D:\WorkSpace\ZZZTouchUI\output\touch-ui` 打开 PowerShell。

```powershell
# 只读解析文件并输出 SHA-256，不启动或注入
.\ZZZTouchLauncher.exe --ZZZ --probe --log

# --probe 的兼容别名
.\ZZZTouchLauncher.exe --ZZZ --probe-auto --log

# 启动默认 Client/3.2；已有 ZZZ 进程时拒绝启动
.\ZZZTouchLauncher.exe --ZZZ

# 开启控制台和 DLL 文件日志
.\ZZZTouchLauncher.exe --ZZZ --log

# 可选：指定另一处游戏
.\ZZZTouchLauncher.exe --ZZZ --game 'D:\Games\ZenlessZoneZero\ZenlessZoneZero.exe'
```

普通权限下执行 GI／SR／ZZZ／WW 的有效启动命令时，会自动弹出 Windows UAC；确认后以管理员身份继续，并保留参数和工作目录。已经以管理员身份运行时直接继续。取消 UAC 会退出，返回 1223，不启动或操作游戏；提权未成功时不会循环弹窗。`--help`、`--probe`、`--probe-auto` 无需提权。

默认路径按项目目录布局寻找 `Client/3.2`；不存在时再寻找旧版目录。游戏安装在其他位置时使用 `--game`。为明确测试 3.2，可执行：

```powershell
.\ZZZTouchLauncher.exe --ZZZ --game 'D:\WorkSpace\ZZZTouchUI\Client\3.2\ZenlessZoneZero.exe'
```

程序使用普通 LoadLibrary 注入；若管理员权限下仍被客户端拒绝加载，应保留错误与日志用于判断。

默认不输出控制台日志，也不创建日志目录或文件；需要诊断时添加 `--log`（包括 `--probe`）。启用后，启动器日志保存在 `logs\launcher-<启动器PID>.log`，窗口关闭后仍可读取。`--help` 始终显示帮助，失败仍返回非零退出码。DLL 日志为 `logs\touch-<PID>.log`。日志开关由本次启动参数决定；修改开关需要退出游戏重新启动。先出现模块基址 `Resolving loaded GameAssembly.dll at base=...`，随后是 `Resolver:` 阶段和全部 15 项解析结果。`DLL loaded` 只表示 DLL 已加载；`READY` 表示输入接口已接入；还应出现 `Effective UI layout=1`。进入可操作场景后依次验证：单指点击、摇杆持续移动、另一指拖动视角、移动期间按技能、抬起全部手指后停止动作、切出再切回。

所有版本都应出现 `resolved all 15 fields from memory`。读取失败会报告对应 RVA，范围不完整时不会用磁盘补齐。解析结果只用于当前进程；版本升级或替换测试版后需完全退出游戏再启动。

串流端必须向 Windows 传递原生触点。只将触屏映射为鼠标或手柄的模式没有独立多指信息，本原型不能从中还原多个触点。若使用 Moonlight/Sunshine，请在你的客户端中选择传递原生触控的模式；不同版本的选项名称可能不同。

不提供指定 PID、附加、状态查询或运行时启停参数。`--game` 仅指定启动路径，不用于选择已有进程。同款游戏在其他安装路径运行时同样拒绝启动。触控在初始化完成后自动启用，故障时停止桥接。DLL 保留在进程内，完全退出游戏即可卸载。工具自身不写入游戏文件或持久配置，UI 设置器引发的游戏内部通知仍需实测确认。

## 根据日志定位

GI 启动窗口很快关闭时，先查看最新的 `logs\launcher-*.log`，其中 `Stage:` 和 `ERROR: stage=` 会指出实际失败步骤。初版 GI 7.0 会因同一组目标的 2 处 UI 调用、4 处输入对象访问被误判为重复而退出；修复版归并等价目标，并继续拒绝冲突目标。

| 现象 | 下一步 |
|---|---|
| 没有日志 | 先确认携带 `--log`，再检查启动器报错、DLL 是否同目录、进程和权限 |
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

`LauncherLog` 验证默认静默、UTF-8 日志路径、标准输出／错误同步落盘、错误和阶段在退出前刷盘，以及独立提权窗口的错误提示策略。`MobileUI` 新增 GI 等价重复调用、不同函数／对象槽／偏移／初始化目标冲突的回归测试。构建脚本可选传入 `-GiSample '<GenshinImpact.exe 或 UserAssembly.dll>'`，使用生产解析器只读验证当前 GI 样本，不启动或执行游戏。

GI／SR UI 片段使用随 VS C++ 工具链提供的 MASM x64 汇编器构建。Python 命令不可用时，可传入 `scripts\build_touch.ps1 -Python 'C:\path\to\python.exe'`。`MobileUI` 覆盖五种特征变体、文件与模拟模块视图、重复／越界拒绝，并在自有内存及自有子进程中执行生产 UI 代码，验证 GI 调用顺序和一次性恢复、SR 周期写入、挂起主线程期间加载系统 DLL、失败清理及成功恢复。仓库存在 `refs/HSR/GameAssembly.dll` 时额外运行 `MobileUIStarRailSample`，只读解析，不执行样本 DLL。

`validate_launcher_cli.py` 覆盖 WW 选择、重复／冲突参数、路径验证、诊断参数拒绝和已有进程检查，普通权限下验证提权失败时不启动游戏；管理员环境下用带空格及中文路径的自有测试 EXE 验证实际收到的两个游戏参数、工作目录与启动器退出后的子进程存活。`MobileUI` 在任意权限下验证 WW 参数与工作目录，`LauncherElevation` 使用模拟 Shell 验证包括 WW 在内的四种游戏选择参数保留。独立启动器测试目录不放置 DLL，自动化检查不请求真实 UAC。

实机待验收：WW 检查移动 UI、移动／视角／按钮操作及触控输入；GI／SR 分别退出后从启动器启动，检查进入游戏后的移动 UI、移动／视角／按钮操作，以及退出后再次启动；ZZZ 回归上述多指动作。测试记录需注明版本、游戏路径、日志和实际可用动作，不能用自动化测试代替实机结果。


构建脚本：`D:\WorkSpace\ZZZTouchUI\scripts\build_touch.ps1`，需要 VS 2022 C++ x64、CMake、Python 3.11+ 和 Python Capstone 包（用于核对生成规则）。脚本使用保留的 2.5/2.6/3.1/3.2 样本运行测试，并复制两个二进制和第三方许可证；它不启动游戏。直接使用 CMake 编译不需要 Python 或 Capstone，HDE64 指令解码器静态编入 EXE/DLL。

`TouchState` 覆盖多指、坐标转换、按帧一致性、快速点击、ID 重用与容量；`TouchBridge31`、`TouchBridge32` 分别在独立进程的隐藏窗口及模拟接口表上测试生产桥接代码，包括原生接口回退、消息转发、重复事件源过滤、取消触点、主线程限制、UI 通知与恢复，以及两版不同的覆盖属性偏移。`profile-validation.json` 将每版 6 个接口槽、3 个 UI 函数的完整导出指令、状态字段及覆盖/默认值调用关系与原始 DLL 交叉核对。

`LauncherElevation` 用模拟 Shell 验证 UAC 取消、错误、循环防护与参数保留，并启动自有测试进程核对真实 CRT 参数解析和退出码传递。覆盖含空格、中文、引号和尾部反斜杠的参数，不弹出真实 UAC。

`ProfileDiscovery25`、`ProfileDiscovery26`、`ProfileDiscovery31`、`ProfileDiscovery32` 分别比较完整文件、模拟加载内存与人工基准的全部 15 项。每版保留 27 类错误候选、重复匹配、ABI 和关系拒绝测试、四类等价指令变形及 setter 移址测试；新增双加载基址、非零槽、不可读槽、删除文件后从内存定位和只修改内存代码的拒绝测试。

`MemoryReader` 覆盖文件偏移与 RVA 分离、跨页面指令、BSS、代码快照独立性，以及 17 类非法内存／PE 头拒绝条件。客户端模拟加载使用自有非执行内存，不调用 `LoadLibrary`，不执行客户端 DLL。`validate_memory_boundary.py` 检查内存解析器的源码依赖、正常路径的单次调用、诊断分支和发布导入；它不是操作系统级文件访问跟踪。

这些检查不等价于真实触摸屏、串流链路或战斗场景验收。WW 传入上述云游戏启动参数；工具不修改游戏持久配置、服务器或账号设置；没有驱动组件或反作弊绕过功能。3.1 研究依据见 `analysis/touch/injection.md`，3.2 地址、字段变化和反编译证据见 `analysis/versions/3.2/touch-adaptation.md`。

人工基准保留在 `profiles/*.json` 与 `analysis/versions/2.5`、`analysis/versions/2.6`。`generate_touch_profiles.py` 分别生成不含地址值的生产字段结构和仅测试使用的 `test_profiles.hpp`；基准与哈希不编入发布程序。原可用版本保存在 `baseline-3.1` 标签和 `releases/baseline-3.1`，其后的固定地址与 v1/v2/v3 发布同样保留。同一进程无法卸载后安全更新 DLL，回退测试前须完全退出游戏。

短布局特征和 Unity 规则仍来自保留的样本；接口派发以数据来源识别成员角色。当前设计与验证见 `analysis/runtime-resolution/memory-resolution.md`，历史数据流设计见 `dataflow-dispatch.md`。统一读取路径不保证任意混淆、内联或指令重写都能识别，也不使用固定地址兜底。当前开发分支为 `codex/gi-sr-mobile-ui`；历史发布归档 `releases/runtime-resolution-v4` 保持原样。分发须保留 `THIRD-PARTY-NOTICES.txt`，其中包含 HDE64 和 GI／SR 参考项目的 MIT 版权声明。
