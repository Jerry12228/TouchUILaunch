[CmdletBinding()]
param(
    [string]$Python = 'python',
    [string]$BuildDir = '',
    [string]$DistDir = '',
    [string]$ZZZSampleRoot = '',
    [string]$SRSample = '',
    [string]$GiSample = ''
)

$ErrorActionPreference = 'Stop'
$nativeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $BuildDir) { $BuildDir = Join-Path $nativeRoot 'build\windows-x64-release' }
if (-not $DistDir) { $DistDir = Join-Path $nativeRoot 'dist' }

# Full verification is deliberately composed from the two independently useful
# entry points, so CI and local development use the same build and test logic.
$buildArguments = @{
    BuildDir = $BuildDir
    DistDir = $DistDir
    ZZZSampleRoot = $ZZZSampleRoot
    SRSample = $SRSample
}
& (Join-Path $PSScriptRoot 'build.ps1') @buildArguments
if ($LASTEXITCODE) { throw 'Minimal build failed' }

$testArguments = @{
    Python = $Python
    BuildDir = $BuildDir
    DistDir = $DistDir
    ZZZSampleRoot = $ZZZSampleRoot
    SRSample = $SRSample
    GiSample = $GiSample
}
& (Join-Path $PSScriptRoot 'test.ps1') @testArguments
if ($LASTEXITCODE) { throw 'Native verification suite failed' }
