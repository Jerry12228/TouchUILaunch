# AI working guide

This guide applies to any coding assistant. It describes project-specific
working practices without requiring a particular model, plugin, or agent
framework. If you have not yet read [AGENTS.md](../AGENTS.md) for this task,
read it once and return here; otherwise continue with this guide. Then read
[Architecture](ARCHITECTURE.md) and choose relevant chapters from the
[index](README.md). Navigation links do not restart the reading sequence or
require recursive traversal. Revisit a document only when its content changes
or the task needs a specific detail again.

## Establish the task and repository

Run commands from the root of this standalone repository. In a containing
workspace, `git status` at the parent level may not describe this checkout.
Confirm the root and existing changes before editing:

```powershell
git rev-parse --show-toplevel
git status --short
git diff --stat
```

Preserve tracked and untracked user work. Do not reset, overwrite, stage, or
commit unrelated changes. Inspect more specific `AGENTS.md` instructions if
they are added under a target directory in the future.

Translate the request into an observable outcome and identify the affected
game, process, and evidence level. A request to diagnose a resolver failure is
different from a request to change its matching rules. A request to document a
known defect does not also request a runtime fix. Inspect discoverable facts
before asking the user for missing information; ask only when the intended
behavior or necessary input remains unclear.

## Trace before editing

Use focused searches rather than loading generated data or all test evidence:

```powershell
rg --files launcher common games scripts docs
rg -n 'game::Kind|game::registry|game::Child' launcher common games
rg -n 'resolve_module|discover\(|install_icalls|maintain_ui' games/zzz
rg -n 'add_test|add_executable|target_link_libraries' CMakeLists.txt
```

For a behavior change, trace the caller, implementation, state or resource
ownership, and tests. Read CMake registration as well as the test source: a
compiled test executable is not necessarily a default CTest case. Read the
actual script parameters before proposing a command.

Useful questions during tracing:

- Is this launcher-side work, code running in the game, or a read-only tool?
- Who owns the child process, handle, allocation, callback, and input state?
- Does the code consume module RVAs, object offsets, absolute addresses, or
  file offsets? Those are not interchangeable.
- Is the edited file handwritten, generated, or test-only evidence?
- Does the failure mean a candidate was rejected, memory could not be read,
  initialization is pending, or a previously installed hook lost ownership?

Keep current behavior and design direction distinct. In particular, the
process layer still depends on game-specific code despite the preferred
boundary for new shared utilities. See
[current dependencies](ARCHITECTURE.md#current-dependencies-and-maintenance-direction).

## Preserve implementation contracts

| Area | Contract to preserve | Why it matters |
| --- | --- | --- |
| Selection and launch | Exactly one game selection; no attachment to already-running games; recheck after acquiring the launch mutex | Avoids duplicate initialization and preserves process ownership |
| Child lifetime | Construct `game::Child` only for a process created by this invocation; release it only after the appropriate successful setup | Failure cleanup must not terminate a discovered or unrelated process |
| Elevation | Validate first, relaunch once, preserve argument quoting and working directory, propagate exit status | Prevents duplicated actions and UAC loops |
| GI/SR runtime | Preserve x64 C++/MASM layouts, argument registers, original behavior, memory protection, and remote allocation lifetime | Code executes in the owned child, with addresses rebased to its modules |
| ZZZ resolver | Resolve once from the loaded module; reject ambiguous or incomplete evidence | Fixed version addresses and disk fallbacks invalidate the memory discovery design |
| ZZZ evidence | Keep manual address oracles and client fingerprints out of production include closures | Fixtures must not become runtime lookup tables |
| ZZZ bridge | Keep UI calls on the window thread and count/GetTouch on one consistent frame source; restore only owned state | Avoids reentrant UI faults, mixed indices, duplicate input, and overwriting other writers |
| Diagnostics | Default silence; explicit `--log`; help/probe do not request elevation | Scripts and tests depend on these public behaviors |

The runtime memory restriction applies to the resolver. The payload can still
write its opt-in log, and the launcher can explicitly probe a disk file.
Do not broaden the restriction into a false claim that the whole application
performs no file I/O. See [ZZZ internals](ZZZ_INTERNALS.md).

The existing product scope excludes frame-rate patches, anti-cheat bypass,
drivers, and modifications to installed game files. Do not import those
features merely because an upstream reference project contains them.

## Implement a focused change

Follow local style and preserve existing public CLI behavior unless the task
changes it. Put per-game signatures and semantics with their owning modules;
keep launch coordination readable. Avoid unrelated reformatting or moving
code just to make the documentation's ideal boundary true.

For a generated artifact, change the owning input or generator, regenerate
explicitly, and inspect both source and output diffs. See
[generated files](DEVELOPMENT.md#generated-files). Do not repair a stale-header
check by editing only the generated header or weakening the check.

For matching changes, demonstrate both the intended acceptance and relevant
rejection behavior. A first-match shortcut is not a substitute for resolving
ambiguity. For lifetime or concurrency changes, inspect failure paths as well
as the success path and select tests that exercise ownership.

## Verify proportionately

Choose checks from [Testing](TESTING.md#choose-checks-for-a-change). Rebuild the
affected binaries before testing them; the verification script does not build
or install fresh artifacts. Run broader checks when the dependency or behavior
change warrants them. Do not add tests that merely restate a documentation edit.

Default tests use owned fixtures and need no game binaries. External-sample
checks read separately supplied inputs. Real game launch and injection are
distinct manual acceptance activities; do not use them as an incidental way
to verify documentation or CLI parsing.

Record commands, results, and skips accurately. If prerequisites are missing,
complete the available checks and explain the unverified part. Neither a
listed test nor a passing sample establishes touch behavior in a live game.
Do not declare real-game compatibility without matching evidence.

## Documentation maintenance

| Changed contract | Documentation to revisit |
| --- | --- |
| CLI, installation, user-visible behavior | Both user READMEs, [Game support](GAME_SUPPORT.md), relevant troubleshooting and CLI checks |
| Module ownership, dependency, process flow | [Architecture](ARCHITECTURE.md), task routing in [index](README.md) |
| Toolchain, scripts, package contents | [Development](DEVELOPMENT.md), [Testing](TESTING.md), concise commands in AGENTS.md |
| Resolver, field schema, generated evidence | [ZZZ internals](ZZZ_INTERNALS.md), generation instructions, sample validation coverage |
| Input ABI, timing, lifecycle, failure handling | [ZZZ internals](ZZZ_INTERNALS.md), touch acceptance and troubleshooting |
| Test registration, sample inputs, evidence level | [Testing](TESTING.md) and chapters that recommend those checks |

Maintain one substantive source for each topic and link to it elsewhere.
Keep `CLAUDE.md` as a navigation shim. Use relative links that work in a
standalone clone; do not make sibling research directories required reading.
When a code defect or stale message is outside the task, document the current
limitation with its source and a valid workaround where one exists. Remove
that limitation after the underlying code changes.

## Git workflow and commit messages

Work like a human maintainer preparing a reviewable patch, not a recorder of
every editing step. A request to implement or review a change does not by
itself authorize committing or pushing it. Do those actions when requested;
keep them scoped to the requested work.

Before preparing a commit, confirm the checkout, current branch, upstream,
recent message style, and intended remote:

```powershell
git rev-parse --show-toplevel
git status --short --branch
git branch -vv
git remote -v
git log -8 --oneline
git diff --stat
git diff
git diff --cached
```

- Inspect untracked files separately; ordinary `git diff` does not show them.
  Preserve unrelated working-tree and staged changes. If a shared file or the
  index cannot be separated safely, ask before committing it.
- Group changes by purpose. Keep a behavior change, its tests, and necessary
  documentation together; split independent changes into separate commits.
  Do not create one commit per file, tool call, or intermediate correction.
- Run the checks appropriate to the final diff and record the actual results.
  Stage explicit paths with `git add -- <paths>`, or selected hunks when a file
  contains unrelated work. Avoid blanket staging such as `git add .` or `-A`
  in a checkout containing other work.
- Before committing, inspect the entire staged patch with `git diff --cached`
  and run `git diff --cached --check`. Confirm `git diff --cached --stat` and
  `git status --short` contain only the intended scope. Do not bypass hooks
  merely to make a commit succeed.

Write messages for the developer who will investigate this change later:

- Use a short, specific English imperative subject, such as
  `Document the shared AI development workflow`. Describe the outcome, not
  `Update files`, `Fix stuff`, or a list of filenames. Match local history;
  no Conventional Commits prefix is mandatory.
- For a nontrivial change, add a blank line and a body explaining why it was
  needed, the important behavior or constraint, and any relevant trade-off.
  Summarize verification and significant skips accurately. Test enumeration
  is not a passing test run; never copy a claim from an example without running
  the check. A self-explanatory small change may need only a subject.
- Use the configured Git author identity. Do not change identity settings or
  invent issue references, sign-offs, co-authors, or test results. Do not add
  tool branding or boilerplate unless explicitly required.

After `git commit`, inspect `git show --stat --oneline HEAD` and the remaining
status. Report the commit ID and any intentionally uncommitted work.

When a push is requested, verify the remote and destination branch, fetch the
relevant remote state, and inspect **all** commits that would be sent, not only
the latest commit. Push with an explicit remote and branch/refspec. If the
destination is ambiguous, the histories diverge, or unrelated unpublished
commits would be included, stop and clarify rather than silently publishing
them or rewriting history. Do not force-push, amend published commits, reset,
rebase other people's work, or discard changes without explicit authorization.
After pushing, confirm the destination ref matches the intended commit; a
local commit alone is not a successful push.

## Handoff

Before finishing, review `git diff --check`, `git diff --stat`, and
`git status --short`; inspect new files as well as tracked diffs. A concise
handoff should state the resulting behavior or artifact, important paths,
verification performed, and any samples or live scenarios not checked.

For code reviews and PRs, identify affected games and explain why the change
is needed. Link an issue if applicable and include screenshots for visible UI
changes. Preserve license attribution. Follow the
[Git workflow](#git-workflow-and-commit-messages) when commits or pushes are
part of the task.

## Portable reading prompt

Use this with a tool that needs an explicit documentation entry point:

> Work in the TouchUILaunch repository root. Read AGENTS.md,
> docs/AI_GUIDE.md, and docs/ARCHITECTURE.md first. Use docs/README.md to select
> the chapters relevant to my task. Inspect current code and tests before
> editing, preserve existing changes, and follow the documented ownership,
> generation, and verification contracts. Report checks actually performed
> and distinguish fixture, sample, and real-game evidence. My task is: ...
