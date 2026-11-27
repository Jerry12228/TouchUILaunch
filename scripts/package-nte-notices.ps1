[CmdletBinding()]
param([Parameter(Mandatory)][string]$AssetsFile, [Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference = 'Stop'
$nativeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$assets = Get-Content -LiteralPath $AssetsFile -Raw | ConvertFrom-Json
$packageFolders = @($assets.packageFolders.PSObject.Properties.Name)
$packages = @{}
foreach ($entry in $assets.libraries.PSObject.Properties) {
    if ($entry.Value.type -eq 'package') { $packages[$entry.Name] = $entry.Value.path }
}
foreach ($framework in $assets.project.frameworks.PSObject.Properties.Value) {
    foreach ($dependency in $framework.downloadDependencies) {
        # NuGet pins runtime downloads as [version, version]. Preserve that
        # resolved version instead of selecting a different installed runtime.
        if ($dependency.version -notmatch '^\[([^,\]]+)') { throw 'Unrecognized runtime package version' }
        $version = $Matches[1].Trim()
        $identity = "$($dependency.name)/$version"
        $packages[$identity] = $identity.ToLowerInvariant()
    }
}
$upstreamLicense = Get-Content -LiteralPath (Join-Path $nativeRoot 'third_party\nte_login_host\LICENSE') -Raw
$mitTerms = $upstreamLicense.Substring($upstreamLicense.IndexOf('Permission is hereby granted'))
foreach ($identity in ($packages.Keys | Sort-Object)) {
    $directory = $null
    foreach ($folder in $packageFolders) {
        $candidate = Join-Path $folder $packages[$identity]
        if (Test-Path -LiteralPath $candidate -PathType Container) { $directory = $candidate; break }
    }
    if (-not $directory) { throw "Restored dependency is missing: $identity" }
    $output = Join-Path $Destination $packages[$identity]
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    $spec = @(Get-ChildItem -LiteralPath $directory -Filter '*.nuspec' -File)
    if ($spec.Count -ne 1) { throw "Dependency metadata is missing: $identity" }
    Copy-Item -LiteralPath $spec[0].FullName -Destination $output
    foreach ($file in (Get-ChildItem -LiteralPath $directory -File)) {
        if ($file.Name -match '(?i)licen[cs]e|notice|copying') {
            Copy-Item -LiteralPath $file.FullName -Destination $output
        }
    }
    # Some NuGet packages declare MIT and a copyright in metadata, but omit
    # a separate license file. Include both the metadata and complete terms.
    [xml]$xml = Get-Content -LiteralPath $spec[0].FullName -Raw
    $metadata = $xml.SelectSingleNode("/*[local-name()='package']/*[local-name()='metadata']")
    $license = $metadata.SelectSingleNode("*[local-name()='license']")
    if ($license -and $license.type -eq 'expression' -and $license.InnerText -eq 'MIT') {
        $copyright = $metadata.SelectSingleNode("*[local-name()='copyright']")
        $notice = "$identity`nMIT License`n$($copyright.InnerText)`n`n$mitTerms"
        Set-Content -LiteralPath (Join-Path $output 'LICENSE-MIT.txt') -Value $notice -Encoding utf8
    }
}
Write-Output "Packaged notices for $($packages.Count) restored NTE host dependencies."
