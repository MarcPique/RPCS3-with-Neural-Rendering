#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CleanStage,
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$SevenZipDirectory = 'C:/Program Files/7-Zip'
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$stage = (Resolve-Path -LiteralPath $CleanStage).Path
$package = [IO.Path]::GetFullPath($PackageDirectory)
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $package) { throw 'PackageDirectory must not already exist.' }
$forbidden = Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object {
    ($_.Name -match '^(ReShade64|nvngx.*|VkLayer_feed_vk)\.dll$|\.addon(32|64)$|\.(fx|fxh|log|pdb)$' -and $_.Name -ne 'rpcs3-settings-only.addon64') -or
    $_.Name -in @('ReShade.ini','ReShadePreset.ini','dlss5-feed.cfg','neural-rendering.json','CurrentSettings.ini')
}
if ($forbidden) { throw 'CleanStage contains runtime components, user settings or debug files.' }
foreach ($required in @('rpcs3.exe','rpcs3-settings-only.addon64','THIRD-PARTY-NOTICES/ReShade-SDK/LICENSE.md','opencv_world4140.dll','Qt6Core.dll','Qt6Gui.dll','Qt6Widgets.dll','Qt6Network.dll',
    'Qt6Multimedia.dll','Qt6MultimediaWidgets.dll','Qt6Svg.dll','Qt6SvgWidgets.dll','Qt6Concurrent.dll',
    'avcodec-61.dll','avformat-61.dll','avutil-59.dll','swresample-5.dll','swscale-8.dll','qt6/plugins/platforms/qwindows.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $required) -PathType Leaf)) { throw "Missing release dependency: $required" }
}
foreach ($required in @('7z.exe','7z.sfx','License.txt')) {
    if (-not (Test-Path -LiteralPath (Join-Path $SevenZipDirectory $required))) { throw "Missing 7-Zip component: $required" }
}
$sourceCommit = (& git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') { throw 'Cannot determine source commit.' }
New-Item -ItemType Directory -Path $package,$output -Force | Out-Null
Get-ChildItem -LiteralPath $stage | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $package -Recurse }
foreach ($name in @('Setup-Neural.ps1','fetch-neural-runtime.ps1')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $package
}
Copy-Item -LiteralPath (Join-Path $repo 'NEURAL_RENDERING.md') -Destination (Join-Path $package 'LEEME.md')
Copy-Item -LiteralPath (Join-Path $repo 'docs/neural-rendering/VALIDACION.md') -Destination $package
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE') -Destination $package
$sevenZipNotice = Join-Path $package 'THIRD-PARTY-NOTICES/7-Zip'
New-Item -ItemType Directory -Path $sevenZipNotice -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $SevenZipDirectory 'License.txt') -Destination $sevenZipNotice
@('Self-extracting module: 7-Zip 26.02, unmodified.',
  'Source: https://github.com/ip7z/7zip/tree/26.02',
  'The EXE contains the same application files as the ZIP and only extracts them.') |
    Set-Content -LiteralPath (Join-Path $sevenZipNotice 'SOURCE.txt') -Encoding utf8
[IO.File]::WriteAllText((Join-Path $package 'neural-rendering.json'), '{"schema":1,"enabled":false}', [Text.UTF8Encoding]::new($false))
foreach ($provenanceFile in Get-ChildItem -LiteralPath (Join-Path $package 'THIRD-PARTY-NOTICES') -Filter 'SOURCE-PROVENANCE.json' -File -Recurse) {
    $provenance = Get-Content -LiteralPath $provenanceFile.FullName -Raw | ConvertFrom-Json
    foreach ($record in $provenance.qtArchiveProvenance) { $record.PSObject.Properties.Remove('destination') }
    $provenance | Add-Member -NotePropertyName forkSourceCommit -NotePropertyValue $sourceCommit -Force
    $provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $provenanceFile.FullName -Encoding utf8
}
$exe = Join-Path $package 'rpcs3.exe'
$buildInfo = [ordered]@{
    release = 'v0.2.2-neural'
    source = "https://github.com/MarcPique/RPCS3-with-Neural-Rendering/tree/$sourceCommit"
    sourceCommit = $sourceCommit
    upstreamBase = '8db660b185496f115701ef4c77c1ca2bef60e422'
    executableSHA256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    build = 'Release x64; MSVC 14.51; Qt 6.11.2; LLVM 22.1.8'
    notes = 'The executable was built before the release commit. The tagged source includes the compiled changes plus release documentation and tools.'
}
$buildInfo | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $package 'BUILD-INFO.json') -Encoding utf8
$zip = Join-Path $output 'RPCS3-Neural-0.2.2-public-win64.zip'
if (Test-Path -LiteralPath $zip) { throw 'Output ZIP already exists.' }
[IO.Compression.ZipFile]::CreateFromDirectory($package, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
$selfExtracting = Join-Path $output 'Extraer-RPCS3-Neural-0.2.2-win64.exe'
if (Test-Path -LiteralPath $selfExtracting) { throw 'Self-extracting EXE already exists.' }
Push-Location (Split-Path $package -Parent)
try {
    & (Join-Path $SevenZipDirectory '7z.exe') a -t7z '-mx=5' ('-sfx' + (Join-Path $SevenZipDirectory '7z.sfx')) $selfExtracting (Split-Path $package -Leaf)
    if ($LASTEXITCODE -ne 0) { throw 'Self-extracting archive creation failed.' }
} finally { Pop-Location }
$sums = foreach ($name in @('RPCS3-Neural-0.2.2-public-win64.zip','Extraer-RPCS3-Neural-0.2.2-win64.exe')) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $output $name) -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $name"
}
$sums | Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Encoding ascii
Write-Host "Public assets prepared at $output"
