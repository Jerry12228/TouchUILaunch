[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [string]$DistDir = '',
    [string]$ZZZSampleRoot = '',
    [string]$SRSample = ''
)

$ErrorActionPreference = 'Stop'
$nativeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $BuildDir) { $BuildDir = Join-Path $nativeRoot 'build\windows-x64-release' }
if (-not $DistDir) { $DistDir = Join-Path $nativeRoot 'dist' }

function Resolve-ProjectPath([string]$Path) {
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $nativeRoot $Path))
}

function Assert-File([string]$Path, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label does not exist or is not a file: $Path"
    }
}

$build = Resolve-ProjectPath $BuildDir
$dist = Resolve-ProjectPath $DistDir

# External samples are opt-in. They are only read by CTest; no executable in
# this project loads or executes them.
if ($SRSample -and -not $ZZZSampleRoot) {
    throw '-SRSample requires -ZZZSampleRoot because external-sample CTest registration is one configuration.'
}
if ($ZZZSampleRoot) {
    $ZZZSampleRoot = [IO.Path]::GetFullPath($ZZZSampleRoot)
    foreach ($version in '2.5', '2.6', '3.1', '3.2') {
        Assert-File (Join-Path $ZZZSampleRoot "$version\GameAssembly.dll") "ZZZ $version sample"
    }
    if ($SRSample) {
        $SRSample = [IO.Path]::GetFullPath($SRSample)
        Assert-File $SRSample 'SR sample'
    }
}

$configure = @('-S', $nativeRoot, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64')
if ($ZZZSampleRoot) {
    $configure += @('-DTOUCHUI_ENABLE_EXTERNAL_SAMPLES=ON', "-DTOUCHUI_ZZZ_SAMPLE_ROOT=$ZZZSampleRoot")
    if ($SRSample) { $configure += "-DTOUCHUI_SR_SAMPLE=$SRSample" }
} else {
    $configure += '-DTOUCHUI_ENABLE_EXTERNAL_SAMPLES=OFF'
}

& cmake @configure
if ($LASTEXITCODE) { throw 'CMake configure failed' }
& cmake --build $build --config Release -- /m:1 /v:minimal
if ($LASTEXITCODE) { throw 'Native build failed' }

New-Item -ItemType Directory -Path $dist -Force | Out-Null
& cmake --install $build --config Release --prefix $dist
if ($LASTEXITCODE) { throw 'Packaging failed' }

Write-Output "Artifacts: $dist"
