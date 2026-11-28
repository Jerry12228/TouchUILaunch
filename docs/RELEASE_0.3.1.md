# TouchUILaunch 0.3.1

本版改善异环（NTE）的串流触摸输入，保留已确认的两项问题。

## 主要变化

- 保留游戏原生触摸回退，启用重复鼠标事件过滤，改善战斗、地图拖动、设置点击／下拉框和滑块操作。
- 改善手机／ESC 菜单的触摸分页，保留游戏原生拖动、捕获及释放流程。
- 触控修正默认启用，普通启动无需 `--log`、额外参数或内部实验环境变量。`--log` 仅增加有数量／时长上限的诊断记录。
- 在游戏主线程启动前保留原生钩子缓冲区，并补充内存分配失败诊断。

## 仍保留的已知问题

以下为国服 Moonlight + Sunshine 原生触摸的实际反馈，本版未修复：

1. **每次打开手机／ESC 菜单后的首次滑动失败**。松开手指再滑动可以翻页；关闭菜单再打开，首次滑动仍会失败。
2. **设置列表在按住拖动时滚动过快**，滚动距离大于手指移动距离。

实体触摸屏和国际服串流触摸未单独验收；国际服本轮仅验证了已有内存样本上的发现逻辑。上述实机反馈来自开发构建；发布整理阶段将同一配置改为默认启用，未另行启动真实游戏。

## 下载与选择

| 发布包 | 内容与适用场景 |
| --- | --- |
| [Standard 标准包](https://github.com/Jerry12228/TouchUILaunch/releases/download/0.3.1/TouchUILaunch-v0.3.1-win-x64-standard.zip) | EXE、共用 DLL、中英文 README、发布说明及许可证。适合其他游戏，以及携带 `--nte-official` 通过官方启动器启动异环。 |
| [NTE Login 异环独立登录包](https://github.com/Jerry12228/TouchUILaunch/releases/download/0.3.1/TouchUILaunch-v0.3.1-win-x64-nte-login.zip) | 标准包全部内容，另附完整的 `nte-login-host/`、运行时与依赖许可证，适合默认的国服／国际服独立登录，也支持标准包全部场景。 |

两个包仍需已安装的游戏。请完整解压并保留目录结构；独立登录包需保留整个 `nte-login-host/`。标准包不包含登录宿主，启动异环时须选择 `--nte-official` 或 `--nte-launcher`，不会自动切换启动方式。

```powershell
# 独立登录包：国际服；国服将 Global 改为 CN
.\TouchUILaunch.exe --NTE --nte-region Global --game "X:\NTE\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe"

# 标准包或独立登录包：官方启动器中转
.\TouchUILaunch.exe --NTE --nte-official --game "X:\NTE\Client\WindowsNoEditor\HT\Binaries\Win64\HTGame.exe"
```

需要诊断时追加 `--log`，独立登录需要全屏时可追加 `--extra '-fullscreen'`。
具体构建、样本和归档验证结果见发布附件 `release.json`；`SHA256SUMS.txt` 提供附件校验值。

## English

This patch improves NTE touch input over streaming. Native touch fallback is
preserved, duplicate mouse events are filtered, and phone paging uses corrected
native capture handling. Corrections run by default, including without logging.
`--log` only adds bounded diagnostics. Early hook-buffer reservation and
allocation-failure diagnostics are also included.

**Known issues remain:** the first swipe each time the phone/ESC menu opens
fails until the finger is released and another swipe begins; settings-list
dragging still scrolls farther than the finger moves. These were observed with
CN native touch through Moonlight + Sunshine. Physical touch and Global streamed
touch were not separately accepted. The release promotion uses the tested
configuration by default and was not followed by another live-game launch.

Choose **Standard** for the other games or official-launcher NTE, and **NTE
Login** for independent CN/Global SDK login. Keep the complete extracted folder.
Both editions still require an installed game; Standard requires an explicit
official NTE route when no login host is included. See `release.json` for the
recorded verification scope and `SHA256SUMS.txt` for attachment hashes.
