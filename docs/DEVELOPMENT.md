# Development and generated files

Run every command below in PowerShell from the repository root. Use
[Testing](TESTING.md) to choose verification and [Troubleshooting](TROUBLESHOOTING.md)
when a prerequisite or command fails.

## Environment

| Requirement | Reason |
| --- | --- |
| Windows x64 | The project rejects other platforms/word sizes and uses Windows process, input, and elevation APIs |
| Visual Studio 2022, Desktop development with C++, Windows SDK, MASM x64 | CMake selects `Visual Studio 17 2022` with architecture `x64`; the stubs use `ASM_MASM` |
| CMake 3.25+ for the documented preset workflow | `CMakePresets.json` uses schema version 6 |
| Python 3.10+ | Generated-schema checks, CLI verification, and binary boundary checks |
| Capstone Python package, only for scan-rule generation/checking | The generator disassembles a preserved 3.1 input; the runtime uses bundled HDE64 |

[CMakeLists.txt](../CMakeLists.txt) declares a 3.24 minimum, and the scripts
configure directly with `-S/-B/-G/-A`. The
[preset file](../CMakePresets.json) also declares 3.24 in its minimum field,
but its schema version 6 requires 3.25 to parse. Use 3.25+ when following preset
examples; these are distinct requirements in the current repository.

Python 3.10+ is required by the default verification workflow: the
[memory boundary checker](../games/zzz/tools/validate_memory_boundary.py) writes
its report with `Path.write_text(..., newline=...)`, added in Python 3.10.

C++ is C++20 without language extensions. MSVC uses `/W4`, `/EHsc`, and
`/utf-8`, with the static runtime (`/MT`, or `/MTd` for Debug). The default
workflow builds Release. Bundled HDE64 is compiled with `/W0`. Do not change
ABI, architecture, runtime linkage, or MASM layout as incidental cleanup.

Inspect the environment without building:

```powershell
cmake --version
cmake --list-presets=all
python --version
```

The default build uses checked-in headers and bundled dependencies. It does
not need game binaries, a metadata dump, Capstone, or files from a parent
workspace. Python is used by verification, not by the CMake compile step.

## Build, package, and verify

The main entry points are [build.ps1](../scripts/build.ps1),
[test.ps1](../scripts/test.ps1), and [full.ps1](../scripts/full.ps1):

```powershell
./scripts/build.ps1
./scripts/test.ps1
```

Or run the composition:

```powershell
./scripts/full.ps1
```

`build.ps1` validates supplied sample paths, configures CMake, builds Release
targets, and runs `cmake --install`. It does not run tests. `test.ps1` requires
existing build/package artifacts and checks them without rebuilding or
regenerating tracked source. `full.ps1` forwards the appropriate options to
both scripts and stops on failure.

| Parameter | Accepted by | Meaning |
| --- | --- | --- |
| `-BuildDir` | All three | Build tree; default `build/windows-x64-release` |
| `-DistDir` | All three | Install/verification directory; default `dist` |
| `-Python` | `test.ps1`, `full.ps1` | Python executable; default `python` |
| `-ZZZSampleRoot` | All three | Root with all four versioned ZZZ sample directories; opts into external CTest registration during build |
| `-SRSample` | All three | Optional SR GameAssembly file; currently requires `-ZZZSampleRoot` |
| `-GiSample` | `test.ps1`, `full.ps1` | Optional GI PE file for a separate resolver check; not a build-script option |

Relative build/dist arguments are resolved against the repository root. Use
absolute sample paths for clarity. Supplying `-ZZZSampleRoot` does not
automatically switch the build directory to the samples preset. Keep external
sample builds separate using the commands in [Testing](TESTING.md#external-sample-tests).

For direct CMake use, these commands build, install, then run native CTest
cases; CTest alone does not include the Python/package checks:

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
cmake --install build/windows-x64-release --config Release --prefix dist
ctest --preset windows-x64-release
```

The Visual Studio generator is multi-configuration. Keep `--config Release`
on direct build/install operations and `-C Release` on direct CTest operations;
the checked-in build/test presets already select Release.

## Artifacts and paths

Build executables reside in `build/windows-x64-release/Release/` by default.
The CMake install rules package:

```text
dist/
  TouchUILaunch.exe
  TouchUILaunch.dll
  README.md
  LICENSE
  THIRD-PARTY-NOTICES.txt
  licenses/hde64/LICENSE.txt
  licenses/gi_sr/LICENSE.txt
```

`test.ps1` additionally writes `dist/memory-boundary.json`. Test executables,
test oracles, this handbook, and the Chinese README are not installed by the
current rules. The installer does not clear an existing destination; old files
may remain there. Inspect the actual install manifest and destination before
describing package contents or distributing a build.

Keep the DLL beside the EXE for ZZZ launch and probe. GI/SR/WW launch paths do
not load this DLL, although the standard package and verification suite expect
both artifacts. See [Game support](GAME_SUPPORT.md).

[.gitignore](../.gitignore) excludes `build/`, `dist/`, Python bytecode, and
`__pycache__/`. Put generated reports under those output roots. A custom output
directory elsewhere is not automatically ignored. Do not commit local game
samples, logs, build trees, or private machine paths.

## Generated files

| Generator and inputs | Outputs | Intended use |
| --- | --- | --- |
| [generate_profiles.py](../games/zzz/tools/generate_profiles.py): `FIELDS` schema plus versioned JSON validation; manual values from 2.5, 2.6, 3.1, 3.2 | [profile.hpp](../games/zzz/include/profile.hpp), [test_profiles.hpp](../games/zzz/include/test_profiles.hpp) | Address-free production schema and separate test-only oracle table |
| [generate_scan_patterns.py](../games/zzz/tools/generate_scan_patterns.py): preserved 3.1 GameAssembly, explicit rule locations, Capstone | [scan_patterns.hpp](../games/zzz/include/scan_patterns.hpp), [patterns.json](../games/zzz/testdata/patterns.json) | Masked production scan rules and development provenance/digests |

### Field schema and manual oracles

The production schema is emitted from the generator's `FIELDS` tuple. The
3.1/3.2 JSON rows are validated against that schema; 2.5/2.6 rows are combined
with them to produce the manual test oracle table. Production `profile.hpp`
contains field names and types, not version-specific values or hashes.

Check without rewriting:

```powershell
python games/zzz/tools/generate_profiles.py --check
```

After an intentional schema/evidence change, regenerate and review:

```powershell
python games/zzz/tools/generate_profiles.py
git diff -- games/zzz/include/profile.hpp games/zzz/include/test_profiles.hpp games/zzz/testdata
python games/zzz/tools/generate_profiles.py --check
```

Update consumers and tests with schema changes. The generator currently names
the old `scripts/generate_touch_profiles.py` path in its stale-file error. Use
the command above; do not recreate the obsolete path. Adding a new version
JSON alone is insufficient because the generator enumerates versions in code.

### Scan rules

Use an explicit, preserved 3.1 sample. Check its SHA-256 against the provenance
in `patterns.json` before regeneration; the generator accepts an explicit path
but does not independently enforce the recorded input hash. It uses fixed
reference locations to extract rules, so another version is not a substitute.

```powershell
Get-FileHash 'C:/Samples/ZZZ/3.1/GameAssembly.dll' -Algorithm SHA256
python games/zzz/tools/generate_scan_patterns.py --game-assembly 'C:/Samples/ZZZ/3.1/GameAssembly.dll' --check
```

When the task intentionally updates the generated rules, omit `--check`:

```powershell
python games/zzz/tools/generate_scan_patterns.py --game-assembly 'C:/Samples/ZZZ/3.1/GameAssembly.dll'
git diff -- games/zzz/include/scan_patterns.hpp games/zzz/testdata/patterns.json
```

The generator masks relocatable references, branch targets, and selected
layout-dependent values; masked bytes are also zeroed. Semantic constants and
ABI checks remain. Source reference addresses and hashes belong to development
evidence, not a production version lookup. Rule regeneration/checking is not
part of `test.ps1`; report it separately and run sample regressions when rules
change. Do not claim it ran merely because field-schema checking passed.

### Static evidence validator

[validate_profile.py](../games/zzz/tools/validate_profile.py) compares known
sample bytes, manual fields, and IDA evidence. It is separate from automatic
discovery tests and is not called by `test.ps1`. Its default input path uses
an undefined `ROOT` symbol. Supply both version and sample directory:

```powershell
python games/zzz/tools/validate_profile.py --version 3.1 --game-dir 'C:/Samples/ZZZ/3.1' --output-dir dist
```

Its normal candidate set contains 3.1/3.2. To inspect a checked-in older oracle,
explicitly supply it through `--extra-profile`:

```powershell
python games/zzz/tools/validate_profile.py --version 2.6 --game-dir 'C:/Samples/ZZZ/2.6' --extra-profile games/zzz/testdata/2.6.json --output-dir dist
```

These commands require the exact matching preserved inputs. Reports are
`profile-validation-<version>.json` and `profile-validation.json` under the
output directory. A passing static report does not prove runtime initialization
or gameplay. See [test data notes](../games/zzz/testdata/README.md).

## Style and dependency maintenance

Use four-space indentation in C++, Python, and PowerShell and two spaces in
CMake; follow neighboring formatting rather than reformatting unrelated code.
Prefer `snake_case` names for files/functions/variables and `PascalCase` for
types. Headers use `.hpp` and `#pragma once`. There is no checked-in formatter
or linter configuration. Keep MASM symbols and C++ layout assertions synchronized.

The project license is [GPL-3.0-only](../LICENSE). HDE64 retains its
[upstream notice](../third_party/hde64/README.md) and
[license](../third_party/hde64/LICENSE.txt). GI/SR adaptations have
[source provenance](../third_party/gi_sr/README.md) and an
[MIT license](../third_party/gi_sr/LICENSE.txt). Preserve these and the bundled
[distribution notices](THIRD-PARTY-NOTICES.txt) when touching derived code.
The recorded upstream snapshot is provenance, not a required sibling checkout.
The build installs existing notice files; update them deliberately when
dependencies change.
