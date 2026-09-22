[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$destination = Join-Path $workspace 'build\third-party-notices'
$repo = Join-Path $workspace 'rpcs3'
$qt = (Get-Content -LiteralPath (Join-Path $workspace 'downloads\qt-ready.txt') -Raw).Trim()
$savedPath = $env:PATH
Remove-Item Env:Path -ErrorAction SilentlyContinue
Remove-Item Env:PATH -ErrorAction SilentlyContinue
$env:Path = 'C:\Program Files\Git\usr\bin;C:\Program Files\Git\mingw64\bin;' + $savedPath
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE') -Destination (Join-Path $destination 'RPCS3-LICENSE.txt') -Force

$thirdParty = Join-Path $repo '3rdparty'
$licenseFiles = Get-ChildItem -LiteralPath $thirdParty -Recurse -File -Force |
    Where-Object { $_.Name -match '^(LICENSE|COPYING|NOTICE|LICENCE)' -and $_.FullName -notmatch '[\\/]\.git[\\/]' } |
    Select-Object -ExpandProperty FullName
foreach ($file in $licenseFiles) {
    $relative = [IO.Path]::GetRelativePath($thirdParty, $file)
    $target = [IO.Path]::GetFullPath((Join-Path (Join-Path $destination 'rpcs3-dependencies') $relative))
    if (-not $target.StartsWith($destination + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Invalid license path: $target" }
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $file -Destination $target -Force
}

$qtNotices = Join-Path $destination 'Qt-6.11.2'
New-Item -ItemType Directory -Path $qtNotices -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $qt 'sbom') -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $qtNotices $_.Name) -Force
}
$extracted = [ordered]@{}
foreach ($jsonFile in (Get-ChildItem -LiteralPath (Join-Path $qt 'sbom') -Filter '*.spdx.json' -File)) {
    $sbom = Get-Content -LiteralPath $jsonFile.FullName -Raw | ConvertFrom-Json
    foreach ($license in $sbom.hasExtractedLicensingInfos) {
        if (-not $extracted.Contains($license.licenseId)) { $extracted[$license.licenseId] = $license.extractedText }
    }
}
$noticeLines = [Collections.Generic.List[string]]::new()
$noticeLines.Add('Third-party license texts supplied in the official Qt 6.11.2 SBOM files.')
foreach ($id in $extracted.Keys) {
    $noticeLines.Add('')
    $noticeLines.Add('===== ' + $id + ' =====')
    $noticeLines.Add([string]$extracted[$id])
}
$noticeLines | Set-Content -LiteralPath (Join-Path $qtNotices 'EXTRACTED-THIRD-PARTY-LICENSES.txt') -Encoding utf8
$index = [ordered]@{
    upstreamRPCS3 = (& git -C $repo rev-parse HEAD)
    qtVersion = '6.11.2'
    llvmVersion = '22.1.8'
    vulkanHeadersVersion = '1.4.341.0'
    submodules = @(& git -C $repo submodule status)
    qtArchiveProvenance = @(Get-ChildItem -LiteralPath (Join-Path $workspace 'downloads') -Filter '*.installed.json' -File | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json })
}
$index | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $destination 'SOURCE-PROVENANCE.json') -Encoding utf8
Write-Host "Notices collected: $destination"
