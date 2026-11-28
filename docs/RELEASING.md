# 发布规范

本文规定 TouchUILaunch 从最终源码到 GitHub Release 的发布流程。适用于维护者和
AI 助手；所有命令均从包含 `CMakeLists.txt` 的仓库根目录执行。

构建接口以[开发说明](DEVELOPMENT.md)及 `scripts/` 为准，测试含义以
[验证说明](TESTING.md)为准。提交、推送和历史保护遵循
[Git 工作约定](AI_GUIDE.md#git-workflow-and-commit-messages)。
撰写文档或准备发布包本身不等于要求公开发布；已有明确发布授权时，完成下述验证后
按授权范围推进，不另设重复确认步骤。

## 1. 版本、标签与发布目标

| 项目 | 约定 |
| --- | --- |
| 版本号 | 使用 `MAJOR.MINOR.PATCH`。当前 `0.x` 阶段，兼容性修复增加 PATCH；新增游戏、显著能力或不兼容的使用方式变化增加 MINOR。 |
| 正式标签 | 与版本号完全一致，例如 `0.3.1`，不带 `v`。 |
| Release 标题 | 正式版本使用同名标题，例如 `0.3.1`。 |
| 预发布 | 使用明确后缀，例如 `0.4.0-rc.1`，并标记 prerelease，不设为 Latest。 |
| 发布分支 | 默认 `master`；发布标签须指向包含本次完整源码和说明的确定提交。 |
| 源码标识 | `release.json` 记录完整提交 SHA，不能只写分支名或缩写。 |

进入 `1.0` 及以后，不兼容的用户接口变更增加 MAJOR；兼容新增能力增加 MINOR，
兼容修复增加 PATCH。

先检查远端已有版本和标签，再选择新版本号。已公开的版本、标签和附件视为不可变：
需要更换代码或二进制时发布新版本，不移动旧标签或静默替换旧包。

版本依据行为变化决定，不以改动文件数量决定。不得将“部分改善”描述为“全部修复”。

## 2. 固定发布包

每次正式发布同时提供以下两个 Windows x64 Release 包。`<version>` 不含前缀 `v`。

| 包 | 固定文件名 | 使用场景 |
| --- | --- | --- |
| Standard 标准包 | `TouchUILaunch-v<version>-win-x64-standard.zip` | 其他游戏，以及通过 `--nte-official` 或 `--nte-launcher` 使用官方启动器的异环。 |
| NTE Login 独立登录包 | `TouchUILaunch-v<version>-win-x64-nte-login.zip` | 标准包全部场景，另支持默认的异环国服／国际服独立 SDK 登录。 |

两个包的 `TouchUILaunch.exe` 和 `TouchUILaunch.dll` 必须来自同一次原生构建，逐字节
一致。绝区零和异环共用一个载荷 DLL，不再分发 `TouchUILaunch.NTE.dll`。

### Standard 必须包含

```text
TouchUILaunch.exe
TouchUILaunch.dll
README.md
README.zh-CN.md
RELEASE-NOTES.md
LICENSE
THIRD-PARTY-NOTICES.txt
licenses/
  hde64/LICENSE.txt
  gi_sr/LICENSE.txt
  minhook/LICENSE.txt
```

### NTE Login 必须额外包含

```text
nte-login-host/                 # 完整的自包含宿主、运行时、资源和依赖
licenses/nte_login_host/         # LICENSE、来源记录及依赖许可证
```

宿主不得只复制 EXE 或少量 DLL，也不得以用户另装 .NET 代替自包含发布。保留生成的
依赖目录结构和第三方声明。Standard 不包含宿主及其专用许可证目录。

两包都需要已安装的游戏；独立登录仍使用安装目录里的区域 SDK／配置，不能承诺代替
官方启动器更新游戏。Standard 缺少宿主时不会自动改用官方启动器；选择方式须在
README 和发布说明中写清楚。

### 不得进入普通发布包的内容

- PDB 调试符号、测试 EXE／夹具、源码分析工具和开发用依赖。
- 游戏客户端文件、内存映像、IDA 数据库、截图及私有研究材料。
- 运行日志、账户／认证数据、缓存及临时目录。
- `memory-boundary.json` 等构建验证报告；验证结果归纳到发布清单。
- 旧版本残留文件或已废弃的独立 NTE 载荷。

ZIP 根目录直接放置上述程序文件，不附加 `dist/`、版本目录或个人工作区路径。
ZIP 成员名必须为相对路径，不得含绝对路径、`..` 穿越路径或重复成员。

## 3. 源码与工作目录准备

1. 检查当前分支、已有修改、远端及待推送的全部提交，保留无关工作。
2. 完成源码、两个用户 README、实现／测试说明及本版本发布说明的核对。
3. 按已授权范围提交最终内容，在干净的已跟踪工作区构建。标签和清单绑定该提交。
4. 将外部样本、工具输出及发布目录保留在 Git 之外。

```powershell
git rev-parse --show-toplevel
git status --short --branch
git remote -v
git fetch origin master --tags
if ($LASTEXITCODE) { throw 'Cannot refresh release refs' }
git log --oneline origin/master..HEAD
git diff
git diff --cached
```

按版本使用单独目录，例如：

```text
dist/releases/<version>/
  install/                     # 构建安装结果，可含验证报告
  standard/                    # 按内容清单组装的干净发布目录
  nte-login/                   # 同上，含完整登录宿主
  assets/                      # 最终上传的四个附件
  archive-check/               # 新建目录，验证解压后的实际文件
```

现有安装步骤只移除废弃的 NTE DLL，不会全面清空旧目录。组包应从本次构建结果按
清单复制，不能把增量 `dist/` 整个压缩。复用目录前确认其内容和用途；不要清理用户的
游戏、账号或其他工作目录。

## 4. 构建与验证

以下版本号仅作示例，执行时替换为本次实际版本。先正常退出支持的游戏及异环登录
宿主／官方启动器；真实游戏会影响 CLI 测试的进程检查前提，不能为跑测试强制结束
已有游戏。

```powershell
$ErrorActionPreference = 'Stop'
$releaseVersion = '0.3.2'
$releaseBuild = 'build/windows-x64-release'
$releaseRoot = Join-Path (Get-Location) "dist/releases/$releaseVersion"
$releaseInstall = Join-Path $releaseRoot 'install'

./scripts/full.ps1 -BuildDir $releaseBuild -DistDir $releaseInstall -NteLoginHost
```

必须保持宿主的锁定依赖恢复及许可证打包。源不可达时可排查网络或使用核对过的
`-NteNugetSource`，不得通过修改锁文件、关闭审计或忽略错误来制造验证成功。

`full.ps1` 只组合构建、安装和测试。当前没有已提交的通用 ZIP／GitHub 发布脚本；
`build/` 中的一次性工具不是新克隆可用的仓库接口。

### 安装结果与发布包的差异

- 当前 CMake 安装规则不复制中文 README 和版本发布说明；组包时分别补入
  `README.zh-CN.md` 和本版本说明，后者在包内命名为 `RELEASE-NOTES.md`。
- 为两个发布目录复制同一组 EXE／DLL、通用文档和基础许可证。
- 只为 NTE Login 加入本次发布的宿主和宿主许可证；排除 PDB 等非分发文件。
- 最终两个目录组装后再分别运行验证：

```powershell
$releaseStandard = Join-Path $releaseRoot 'standard'
$releaseLogin = Join-Path $releaseRoot 'nte-login'

./scripts/test.ps1 -BuildDir $releaseBuild -DistDir $releaseStandard
./scripts/test.ps1 -BuildDir $releaseBuild -DistDir $releaseLogin -NteLoginHost
```

验证必须覆盖当前注册的完整 CTest、CLI、生成文件和内存边界检查，以及独立登录包的
SDK／资源和托管宿主中转检查。测试数量取自本次实际运行结果，不照抄上一版数字。

外部游戏样本和真实游戏验收单独记录。夹具、样本匹配、UI Ready、成功登录、进入
世界和实际触控操作属于不同证据，不能相互代替。记录地区、客户端版本、触摸来源
及已验证操作；未运行的检查明确写为 skipped 或 not run，并注明原因。

若开发验收依赖实验环境变量或 `--log`，整理普通启动行为后须验证它们不再是功能
生效前提。未对整理后的包重新实测时，应明确披露，不沿用旧实测结果假称已验收。

## 5. 归档与解压验收

为两个干净发布目录生成 ZIP 后，逐包完成以下检查：

1. 内容符合第二节的清单、宿主差异及排除规则，第三方声明完整。
2. ZIP CRC 检查通过；成员路径合法、无重复项。
3. ZIP 内每个文件与本次发布目录的文件内容／SHA-256 一致。
4. 解压到新的验证目录，再比对文件清单与内容；不能只检查原始安装目录。
5. 两个解压包中的 EXE／DLL SHA-256 相同。
6. 从两个解压目录分别运行 `TouchUILaunch.exe --help`，确认正常退出及所需参数。
7. 从解压后的 NTE Login 运行宿主 `--verify-integration` 和 `--verify-resources`。

帮助和宿主离线检查不应触发 UAC、账号登录或真实游戏启动。归档检查失败须修正后
重新生成和复验，不能只重算校验和。

构建后修改运行源码或构建配置，必须重建受影响产物并补跑相关验证；修改随包文档、
压缩内容或文件名，也必须重新生成 ZIP 并核对内容和摘要。

## 6. 发布清单与校验文件

默认每个 Release 上传四个附件：

```text
TouchUILaunch-v<version>-win-x64-standard.zip
TouchUILaunch-v<version>-win-x64-nte-login.zip
release.json
SHA256SUMS.txt
```

`release.json` 至少记录以下信息。字段可扩展，但不得删除验证范围或用模板中的数字
代替实际结果；[0.3.1 清单](https://github.com/Jerry12228/TouchUILaunch/releases/download/0.3.1/release.json)
仅是格式参考，不是后续版本的验证结论。

| 字段 | 要求 |
| --- | --- |
| `version`、`tag`、`repository` | 与发布版本、远端标签和目标仓库一致。 |
| `source_commit` | 标签最终指向的完整源码提交 SHA。 |
| `platform`、`configuration` | 当前为 `windows-x64`、`Release`。 |
| `artifacts` | 两个 ZIP 的名称、edition、字节大小、SHA-256、文件数量。 |
| `shared_native_binaries_sha256` | 两包共用 EXE／DLL 的 SHA-256。 |
| `known_issues` | 本版保留的缺陷、触发条件及有效的临时操作方式。 |
| `validation` | 本次通过／失败情况、测试数量、样本范围、解压检查、实机证据和跳过项。 |

所有 SHA-256 使用文件实际字节计算，以小写十六进制记录。完成两个 ZIP 和
`release.json` 后，生成 `SHA256SUMS.txt`，每行格式为：

```text
<sha256><两个空格><附件文件名>
```

校验文件包含两个 ZIP 和 `release.json`，不包含自身。重写任一被校验附件后，必须
重新生成校验文件，并重新核对上传内容。

## 7. 发布说明与已知问题

本版本说明保存为 `docs/RELEASE_<version>.md`，用于 GitHub Release 正文及包内
`RELEASE-NOTES.md`。正文以中文为主，提供英文摘要，至少包含：

- 用户可观察到的改动、受影响游戏、默认行为或命令变化。
- 两包差异、选择方法和正确的启动示例。
- 保留的缺陷、触发条件、适用地区／输入来源，以及确实有效的临时操作方式。
- 本次实际验证与未验收范围；区分开发会话验收和最终发布产物验收。

允许在用户明确接受保留缺陷的范围内发布补丁，但必须在 README、实现说明和发布
正文中一致披露，不能标记为已修复。0.3.1 的手机首次滑动失败和设置拖动过快是这种
处理方式的实例，不是所有未来版本都必须保留的固定文本。

构建／必需检查失败、内容缺失、提交绑定错误或摘要不一致属于发布阻断，不能以
“已知问题”说明替代修正。某地区或输入设备未实测本身不等于所有发布都被阻断；
应保留准确的 skipped 范围，并遵循已经确定的发布授权。

## 8. 推送、草稿与公开发布

完成本地验证后，确认 `master`、发布提交和待推送的全部历史符合授权。采用正常
合并／快进，不强推、不改写已有发布标签。以下命令要求当前已位于最终发布的
`master`，且两个 ZIP、清单、校验文件和发布说明均已完成验证：

```powershell
$releaseRepository = 'Jerry12228/TouchUILaunch'
$releaseCommit = (git rev-parse HEAD).Trim()
$releaseAssets = Join-Path $releaseRoot 'assets'
$releaseNotes = "docs/RELEASE_$releaseVersion.md"

if (git status --porcelain) { throw 'Release worktree is not clean' }
$releaseManifest = Get-Content -LiteralPath (Join-Path $releaseAssets 'release.json') -Raw | ConvertFrom-Json
if ($releaseManifest.source_commit -ne $releaseCommit -or
    $releaseManifest.version -ne $releaseVersion -or $releaseManifest.tag -ne $releaseVersion) {
    throw 'Release manifest does not match the source commit/version'
}
if (git tag --list $releaseVersion) { throw 'Release tag already exists' }
git tag -a $releaseVersion $releaseCommit -m "TouchUILaunch $releaseVersion"
if ($LASTEXITCODE) { throw 'Cannot create release tag' }
git push --atomic origin master "refs/tags/$releaseVersion"
if ($LASTEXITCODE) { throw 'Release push failed; inspect remote refs before retrying' }
git ls-remote origin refs/heads/master "refs/tags/$releaseVersion" "refs/tags/$releaseVersion^{}"
```

核对远端 `master` 和标签指向预期提交；对于带注释的标签，核对其解引用后的提交，
不能把标签对象 SHA 当作源码提交 SHA。原子推送失败时先检查远端实际状态，不能
直接强推或覆盖已经存在的标签。

随后创建草稿并上传经过验证的四个附件：

```powershell
$releaseUpload = @(
    (Join-Path $releaseAssets "TouchUILaunch-v$releaseVersion-win-x64-standard.zip")
    (Join-Path $releaseAssets "TouchUILaunch-v$releaseVersion-win-x64-nte-login.zip")
    (Join-Path $releaseAssets 'release.json')
    (Join-Path $releaseAssets 'SHA256SUMS.txt')
)

gh release create $releaseVersion @releaseUpload `
    --repo $releaseRepository --verify-tag --target master `
    --title $releaseVersion --notes-file $releaseNotes --draft
if ($LASTEXITCODE) { throw 'Draft creation/upload failed; inspect remote release state' }
```

公开前用 GitHub API 或 CLI 核对标签、正文、known issues、附件名称、数量、大小和
uploaded 状态。服务端 SHA-256 必须与本地一致；若接口不提供摘要，则下载附件再
计算，不能仅凭上传命令退出成功判定文件正确。

核对通过后公开正式版本并设为 Latest：

```powershell
gh release edit $releaseVersion --repo $releaseRepository --draft=false --latest
if ($LASTEXITCODE) { throw 'Release publication failed; verify the actual server state' }
gh release view $releaseVersion --repo $releaseRepository `
    --json tagName,isDraft,isPrerelease,publishedAt,url,assets
gh api "repos/$releaseRepository/releases/latest" --jq '{tag_name,html_url}'
```

预发布须在创建时增加 `--prerelease`，公开时使用 `--latest=false`，不执行上述正式
版本的 Latest 断言。若创建或上传中断，先检查草稿和附件的实际状态，再恢复同一
发布；不要重复创建版本或顺带删除其他发布。

## 9. 发布完成条件

- 远端分支、标签、`source_commit` 和最终源码提交一致，已跟踪工作区干净。
- Release 已公开，正式／预发布标记正确；正式版本的 Latest 指向本次版本。
- 四个附件可见，服务端大小和内容摘要与本地校验一致，下载链接来自实际发布结果。
- README、随包说明、Release 正文和清单对功能、已知问题、验证范围的描述一致。
- 向用户报告版本链接、提交 SHA、两个下载入口、主要改动、保留问题及验证结果。

发布完成后不在原版本下替换二进制。后续文档维护与下一次代码发布分别处理，依照
各自任务范围执行。
