[CmdletBinding()]
param(
    [string]$Python = 'python',
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\build\windows-x64-release'),
    [string]$DistDir = (Join-Path $PSScriptRoot '..\dist'),
    [string]$ZZZSampleRoot = '',
    [string]$SRSample = '',
    [string]$GiSample = ''
)

$ErrorActionPreference = 'Stop'
$nativeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
function Resolve-ProjectPath([string]$Path) {
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $nativeRoot $Path))
}
$build = Resolve-ProjectPath $BuildDir
$dist = Resolve-ProjectPath $DistDir

# Generation checks are read-only: a developer must intentionally regenerate
# headers after changing the checked-in ZZZ test data.
& $Python (Join-Path $nativeRoot 'games\zzz\tools\generate_profiles.py') --check
if ($LASTEXITCODE) { throw 'Generated ZZZ profile headers are stale' }

$configure = @('-S', $nativeRoot, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64')
if ($ZZZSampleRoot) {
    $configure += @('-DTOUCHUI_ENABLE_EXTERNAL_SAMPLES=ON', "-DTOUCHUI_ZZZ_SAMPLE_ROOT=$ZZZSampleRoot")
    if ($SRSample) { $configure += "-DTOUCHUI_SR_SAMPLE=$SRSample" }
}
& cmake @configure
if ($LASTEXITCODE) { throw 'CMake configure failed' }
& cmake --build $build --config Release -- /m:1 /v:minimal
if ($LASTEXITCODE) { throw 'Native build failed' }
& ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'CTest failed' }

if ($GiSample) {
    & (Join-Path $build 'Release\MobileUITests.exe') --gi-file $GiSample
    if ($LASTEXITCODE) { throw 'GI read-only resolution failed' }
}

$launcher = Join-Path $build 'Release\TouchUILaunch.exe'
& $Python (Join-Path $nativeRoot 'launcher\tests\validate_cli.py') $launcher
if ($LASTEXITCODE) { throw 'Launcher CLI validation failed' }

New-Item -ItemType Directory -Path $dist -Force | Out-Null
& cmake --install $build --config Release --prefix $dist
if ($LASTEXITCODE) { throw 'Packaging failed' }
& $Python (Join-Path $nativeRoot 'games\zzz\tools\validate_memory_boundary.py') --output-dir $dist
if ($LASTEXITCODE) { throw 'ZZZ memory-boundary validation failed' }

Write-Output "Artifacts: $dist"
