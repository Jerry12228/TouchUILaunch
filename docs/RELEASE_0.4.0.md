# TouchUILaunch 0.4.0

本版新增《明日方舟：终末地》原生触控 UI 支持，使用游戏已有的触摸输入后端。

## 主要变化

- 新增 `--EF --game <Endfield.exe>`，支持自动查找已安装的官方 GRYPHLINK 启动器；
  自定义位置可携带 `--ef-launcher <Games.exe>`。
- 校验官方启动器的 x64 架构、签名与产品信息，创建本次启动专用的官方父进程，
  在游戏主线程运行前加载共用载荷，解决此前直接启动时出现的官方启动器提示和
  后期载荷加载失败。不会接管已有游戏或已有官方启动器进程。
- 通过运行时 IL2CPP 元数据定位游戏输入接口，在已附加的 Unity 窗口线程调用
  `DeviceInfo.ChangeInputType`，确认实际状态为 Touch 后报告 UI 就绪。
- 默认尝试使用独立的鼠标／手指 UI 指针记录，针对菜单及剧情点击偶发仅产生
  悬浮反馈的问题。该能力有独立的应用／失败日志，缺失时不阻止触控 UI 切换。
- 新增启动、元数据、状态确认、主线程调度、载荷互斥激活及 UI 指针回归夹具，
  补齐中英文用法、故障排查及实机复测说明。

终末地继续使用共用的 `TouchUILaunch.dll`，不需要另一个游戏专用 DLL 或异环登录宿主。
触控 UI 切换与指针策略自动启用；`--log` 仅增加诊断，不是功能开关。

## 下载与选择

| 发布包 | 内容与适用场景 |
| --- | --- |
| [Standard 标准包](https://github.com/Jerry12228/TouchUILaunch/releases/download/0.4.0/TouchUILaunch-v0.4.0-win-x64-standard.zip) | EXE、共用 DLL、中英文 README、发布说明及许可证。适合终末地等游戏，以及携带 `--nte-official` 或 `--nte-launcher` 通过官方启动器启动异环。 |
| [NTE Login 异环独立登录包](https://github.com/Jerry12228/TouchUILaunch/releases/download/0.4.0/TouchUILaunch-v0.4.0-win-x64-nte-login.zip) | 标准包全部内容，另附完整的 `nte-login-host/`、运行时与依赖许可证；支持默认的国服／国际服独立登录，也支持终末地和其他标准包场景。 |

两个包仍需已安装的游戏。请完整解压，同一构建的 EXE／DLL 保持在一起；
独立登录包需保留整个 `nte-login-host/`。标准包没有异环独立登录宿主，启动异环时
必须选择官方中转方式，不会自动切换。官方游戏／启动器文件不包含在发布包中。

## 终末地启动

先通过官方 GRYPHLINK 安装／更新游戏，再正常退出游戏。在解压目录运行：

```powershell
.\TouchUILaunch.exe --EF --game 'C:\Games\Endfield\Endfield.exe' --log
```

默认在 `%ProgramFiles%\GRYPHLINK` 下查找官方 `Games.exe`。安装在其他位置时，
替换以下两个示例路径：

```powershell
.\TouchUILaunch.exe --EF --game 'C:\Games\Endfield\Endfield.exe' --ef-launcher 'D:\Apps\GRYPHLINK\1.6.0\Games.exe' --log
```

同意 Windows 管理员授权后等待 `Endfield touch UI state confirmed`。
**保持 TouchUILaunch 运行直到游戏退出**，以保留本次创建的官方父进程；
无需再手动点击官方启动器中的“开始游戏”。初始化允许等待 GameAssembly 120 秒，
再等待 UI 状态 120 秒。新 DLL 只在下次正常启动时生效，不支持附加已有游戏。

触摸屏或串流客户端需要向 Windows／游戏传递原生触摸事件；本实现不把鼠标转换为
手指。UI Ready、指针钩子就绪、实际应用及可用的控件点击是不同的观测结果。

## 已知问题与验收范围

- **终末地菜单／剧情点击仍待复测**：开发阶段用户已确认触控 UI、移动、视角与
  主控队员切换正常，同时反馈部分菜单、剧情分支和推进对话偶发点击无效，显示
  鼠标悬浮样式。新指针策略已加入回归夹具，但不能据此宣称实机问题已修复。
- 终末地实体／串流触摸、多指、键鼠混用、广泛菜单与场景切换未单独验收。
  最终发布包未另行启动真实游戏；上述基础操作反馈来自此前的启动／UI 开发构建。
- 异环延续已有两项问题：国服 Moonlight + Sunshine 原生触摸下，每次打开手机／ESC
  菜单后的首次滑动仍失败，松开再滑可翻页；设置列表按住拖动时滚动距离仍过大。
  本版不将它们标记为已修复，实体触摸屏与国际服串流触摸未单独验收。

终末地问题反馈请保留提权前后 `logs/launcher-<pid>.log`、`logs/ef-<游戏pid>.log`，
并记录游戏版本、输入设备／串流软件、具体页面控件、操作顺序与失败次数。

## 发布验证

最终发布构建的原生 CTest、完整 CLI、生成文件与内存边界检查，登录宿主的锁定
依赖恢复、离线 SDK／资源与托管中转检查，以及两个 ZIP 的 CRC、逐文件内容、
解压和启动帮助／宿主检查，均在附件 `release.json` 中记录本次实际结果与数量。
两包使用相同的原生 EXE／DLL，不含 PDB、运行日志或游戏文件。

本版未重跑外部 GI／ZZZ／SR 样本，也未新建真实游戏会话。夹具通过不等于所有游戏
版本或触摸设备均已兼容。附件摘要见 `SHA256SUMS.txt`，完整源码提交见 `release.json`。

## English

This release adds Arknights: Endfield support with `--EF --game <Endfield.exe>`.
The installed, signed x64 GRYPHLINK launcher is detected automatically or
selected with `--ef-launcher <Games.exe>`. A newly owned official parent and
early shared-DLL loading replace the failed direct/late-load startup path.
Touch UI is selected through dynamically resolved IL2CPP metadata on the
attached Unity window thread and confirmed by reading the actual input state.

**Keep TouchUILaunch running until the game exits.** No manual Play click in
the official launcher is required. Native Windows/game touch delivery is used;
mouse input is not converted to fingers. Both editions support Endfield without
the NTE login host. Corrections run by default; `--log` only adds diagnostics.

An independent mouse/finger UI pointer policy targets intermittent menu/story
taps that only produce hover feedback. **Its real-game improvement still needs
retesting.** Earlier developer-session feedback confirmed UI, movement, camera
and controlled-member switching. Physical/streamed touch, multitouch, mixed
input and broad scene/menu coverage were not separately accepted. No new live
game session was started for the final release packages.

Existing NTE issues remain: the first swipe after opening the phone/ESC menu
fails until release/retry, and settings dragging scrolls too far. These were
observed with CN native touch via Moonlight + Sunshine; physical touch and
Global streamed touch were not separately accepted.

Choose Standard for Endfield, other games or official-launcher NTE. Choose NTE
Login for independent CN/Global NTE login, retaining its complete host folder.
Both require an installed game. See `release.json` for the actual build,
fixture, CLI, package and offline-host verification results and skipped checks;
`SHA256SUMS.txt` contains the attachment hashes.
