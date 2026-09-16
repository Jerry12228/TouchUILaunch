[CmdletBinding()]
param(
    [ValidateSet('GI', 'SR', 'ZZZ', 'WW')]
    [string]$Game = 'ZZZ',
    [Parameter(Mandatory)]
    [string]$GamePath,
    [string]$DistDir = (Join-Path $PSScriptRoot '..\dist')
)

$ErrorActionPreference = 'Stop'
$dist = (Resolve-Path $DistDir).Path
$launcher = Join-Path $dist 'TouchUILaunch.exe'
$logs = Join-Path $dist 'logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$report = Join-Path $logs ("runtime-$Game-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

& $launcher "--$Game" --game $GamePath --log *>> $report
"Launcher exit code: $LASTEXITCODE" | Add-Content -LiteralPath $report -Encoding utf8
if ($LASTEXITCODE) { exit $LASTEXITCODE }
