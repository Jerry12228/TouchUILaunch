[CmdletBinding()]
param([string]$DistDir = 'dist', [string]$NugetSource = '')
$ErrorActionPreference = 'Stop'
$nativeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$vendor = Join-Path $nativeRoot 'third_party\nte_login_host'
$project = Join-Path $vendor 'src\Hi3Helper.Plugin.NTE.LoginHost\Hi3Helper.Plugin.NTE.LoginHost.csproj'
$dist = if ([IO.Path]::IsPathRooted($DistDir)) { [IO.Path]::GetFullPath($DistDir) } else { Join-Path $nativeRoot $DistDir }
Push-Location $vendor
try {
    $restore = @('restore', $project, '--locked-mode')
    if ($NugetSource) { foreach ($source in $NugetSource.Split(';')) { $restore += @('--source', $source) } }
    & dotnet @restore
    if ($LASTEXITCODE) { throw 'NTE login host package restore failed' }
    & dotnet publish $project -c Release -r win-x64 --self-contained true --no-restore -o (Join-Path $dist 'nte-login-host')
    if ($LASTEXITCODE) { throw 'NTE login host build failed' }
} finally { Pop-Location }
New-Item -ItemType Directory -Path (Join-Path $dist 'licenses\nte_login_host') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $vendor 'LICENSE') -Destination (Join-Path $dist 'licenses\nte_login_host\LICENSE')
Copy-Item -LiteralPath (Join-Path $vendor 'UPSTREAM.json') -Destination (Join-Path $dist 'licenses\nte_login_host\UPSTREAM.json')
& (Join-Path $nativeRoot 'scripts\package-nte-notices.ps1') `
    -AssetsFile (Join-Path (Split-Path $project) 'obj\project.assets.json') `
    -Destination (Join-Path $dist 'licenses\nte_login_host\packages')
Write-Output "NTE independent login host: $dist\nte-login-host\Nte.LoginHost.exe"
