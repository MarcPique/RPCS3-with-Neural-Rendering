[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$repoBin = Join-Path $workspace 'rpcs3\bin'
$stage = Join-Path $workspace 'build\rpcs3-release-stage'
$deps = Get-Content -LiteralPath (Join-Path $workspace 'downloads\build-environment.json') -Raw | ConvertFrom-Json
$exe = Join-Path $repoBin 'rpcs3.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'Release executable does not exist yet.' }
if (-not [IO.Path]::GetFullPath($stage).StartsWith($workspace + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Stage must remain inside workspace.' }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
foreach ($item in (Get-ChildItem -LiteralPath $repoBin -Force)) {
    if ($item.Name -eq 'test' -or $item.Extension -in @('.pdb','.exp','.lib') -or $item.Name -eq 'vc_redist.x64.exe') { continue }
    Copy-Item -LiteralPath $item.FullName -Destination $stage -Recurse -Force
}
# RPCS3 requires the system VC++ redistributable and rejects app-local CRT DLLs.
$notices = Join-Path $workspace 'build\third-party-notices'
if (Test-Path -LiteralPath $notices) {
    $noticeDestination = Join-Path $stage 'THIRD-PARTY-NOTICES'
    New-Item -ItemType Directory -Path $noticeDestination -Force | Out-Null
    foreach ($item in Get-ChildItem -LiteralPath $notices) {
        Copy-Item -LiteralPath $item.FullName -Destination $noticeDestination -Recurse -Force
    }
}
foreach ($required in @('rpcs3.exe','Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','Qt6Svg.dll','Qt6Multimedia.dll','qt6\plugins\platforms\qwindows.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $required))) { throw "Incomplete deployed runtime: $required" }
}
Get-FileHash -LiteralPath (Join-Path $stage 'rpcs3.exe') -Algorithm SHA256
Write-Host "Release stage ready: $stage"
