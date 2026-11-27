[CmdletBinding()]
param(
    [string]$Python = 'python',
    [string]$BuildDir = '',
    [string]$DistDir = '',
    [string]$ZZZSampleRoot = '',
    [string]$SRSample = '',
    [string]$GiSample = '',
    [switch]$NteLoginHost
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
if (-not (Test-Path -LiteralPath $build -PathType Container)) {
    throw "Build directory does not exist: $build. Run scripts\\build.ps1 or scripts\\full.ps1 first."
}
Assert-File (Join-Path $build 'Release\TouchUILaunch.exe') 'Built launcher'
Assert-File (Join-Path $dist 'TouchUILaunch.exe') 'Packaged launcher'
Assert-File (Join-Path $dist 'TouchUILaunch.dll') 'Packaged payload DLL'
if (Test-Path -LiteralPath (Join-Path $dist 'TouchUILaunch.NTE.dll')) {
    throw 'Legacy separate NTE payload remains in package; rebuild with scripts\build.ps1.'
}

# The check deliberately does not regenerate source.  Updating generated rules
# remains an explicit developer action because it requires the 3.1 sample and Capstone.
& $Python (Join-Path $nativeRoot 'games\zzz\tools\generate_profiles.py') --check
if ($LASTEXITCODE) { throw 'Generated ZZZ profile headers are stale' }

if ($SRSample -and -not $ZZZSampleRoot) {
    throw '-SRSample requires -ZZZSampleRoot; rebuild with scripts\\full.ps1 using both options.'
}
if ($ZZZSampleRoot) {
    $ZZZSampleRoot = [IO.Path]::GetFullPath($ZZZSampleRoot)
    foreach ($version in '2.5', '2.6', '3.1', '3.2') {
        Assert-File (Join-Path $ZZZSampleRoot "$version\GameAssembly.dll") "ZZZ $version sample"
    }
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
    if ($cache -notmatch 'TOUCHUI_ENABLE_EXTERNAL_SAMPLES:BOOL=ON') {
        throw 'This build was not configured for external samples. Rebuild with scripts\\full.ps1 -ZZZSampleRoot <path>.'
    }
    if ($SRSample) { Assert-File ([IO.Path]::GetFullPath($SRSample)) 'SR sample' }
} else {
    Write-Output 'SKIP: external ZZZ/SR sample tests (no -ZZZSampleRoot supplied).'
}

& ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'CTest failed' }

if ($NteLoginHost) {
    $hostExe = Join-Path $dist 'nte-login-host\Nte.LoginHost.exe'
    Assert-File $hostExe 'NTE independent login host'
    $hostTest = Start-Process -FilePath $hostExe -ArgumentList '--verify-integration' -WindowStyle Hidden -Wait -PassThru
    if ($hostTest.ExitCode) { throw 'NTE SDK offline integration checks failed' }
    $resourceTest = Start-Process -FilePath $hostExe -ArgumentList '--verify-resources' -WindowStyle Hidden -Wait -PassThru
    if ($resourceTest.ExitCode) { throw 'NTE login host resource checks failed' }
    & (Join-Path $build 'Release\NteRelayTests.exe') --managed $hostExe
    if ($LASTEXITCODE) { throw 'NTE managed-host process relay checks failed' }
} else { Write-Output 'SKIP: optional NTE SDK host checks (no -NteLoginHost supplied).' }

if ($GiSample) {
    $GiSample = [IO.Path]::GetFullPath($GiSample)
    Assert-File $GiSample 'GI sample'
    & (Join-Path $build 'Release\GITouchTests.exe') --sample $GiSample
    if ($LASTEXITCODE) { throw 'GI signature discovery read-only verification failed' }
} else {
    Write-Output 'SKIP: GI sample resolution (no -GiSample supplied).'
}

& $Python (Join-Path $nativeRoot 'launcher\tests\validate_cli.py') (Join-Path $build 'Release\TouchUILaunch.exe')
if ($LASTEXITCODE) { throw 'Launcher CLI validation failed' }
& $Python (Join-Path $nativeRoot 'games\zzz\tools\validate_memory_boundary.py') --output-dir $dist
if ($LASTEXITCODE) { throw 'ZZZ memory-boundary validation failed' }

Write-Output 'PASS: native verification suite completed.'
