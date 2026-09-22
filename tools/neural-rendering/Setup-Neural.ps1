#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$RpcS3Directory = $PSScriptRoot,
    [string]$CacheDirectory = '',
    [int]$FromRpcS3Id = 0
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$target = (Resolve-Path -LiteralPath $RpcS3Directory).Path
$exe = Join-Path $target 'rpcs3.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Run this script beside rpcs3.exe, or pass -RpcS3Directory.' }
if ($FromRpcS3Id) {
    $caller = Get-Process -Id $FromRpcS3Id -ErrorAction Stop
    if (-not [string]::Equals($caller.Path, $exe, [StringComparison]::OrdinalIgnoreCase)) { throw 'The caller is not this copy of RPCS3.' }
    $loaded = $caller.Modules | Where-Object { $_.ModuleName -in @('ReShade64.dll','VkLayer_feed_vk.dll','renodx-dlss5.addon64','dlss5-feed.addon64') }
    if ($loaded) { throw 'Disable ReShade, save and restart RPCS3 before repairing loaded components.' }
}
foreach ($process in (Get-Process rpcs3 -ErrorAction SilentlyContinue)) {
    if ($process.Path -and [string]::Equals($process.Path, $exe, [StringComparison]::OrdinalIgnoreCase)) {
        if ($process.Id -ne $FromRpcS3Id) { throw 'Close other instances of this copy of RPCS3 before installing components.' }
    }
}
if (-not $CacheDirectory) { $CacheDirectory = Join-Path $target '.neural-downloads' }
$stage = Join-Path $target ('.neural-setup-' + [Guid]::NewGuid().ToString('N'))
$runtime = Join-Path $stage 'runtime'
$helper = Join-Path $stage 'diagnostic-helper'

# Download and validate first. Never execute downloaded code or alter Vulkan's registry.
& (Join-Path $PSScriptRoot 'fetch-neural-runtime.ps1') -RuntimeDirectory $runtime -TestHelperDirectory $helper -CacheDirectory $CacheDirectory
foreach ($name in @('ReShade64.dll','dlss5-feed.addon64','renodx-dlss5.addon64','nvngx_dlssnr.dll','nvngx_dlss.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $runtime $name) -PathType Leaf)) { throw "Missing downloaded component: $name" }
}

# Managed components are replaceable; existing user settings are retained.
foreach ($name in @('ReShade64.dll','dlss5-feed.addon64','renodx-dlss5.addon64','nvngx_dlssnr.dll','nvngx_dlss.dll','LOCAL-USE-NOTICE.txt','PROVENANCE.json')) {
    Copy-Item -LiteralPath (Join-Path $runtime $name) -Destination (Join-Path $target $name) -Force
}
foreach ($directory in @('neural-rendering','reshade-shaders','licenses')) {
    $sourceDirectory = Join-Path $runtime $directory
    $destination = Join-Path $target $directory
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($item in Get-ChildItem -LiteralPath $sourceDirectory) {
        Copy-Item -LiteralPath $item.FullName -Destination $destination -Recurse -Force
    }
}
if (-not $FromRpcS3Id) {
foreach ($name in @('ReShade.ini','ReShadePreset.ini','dlss5-feed.cfg')) {
    if (-not (Test-Path -LiteralPath (Join-Path $target $name))) {
        Copy-Item -LiteralPath (Join-Path $runtime $name) -Destination (Join-Path $target $name)
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $target 'neural-rendering.json'))) {
    [IO.File]::WriteAllText((Join-Path $target 'neural-rendering.json'), '{"schema":1,"enabled":false}', [Text.UTF8Encoding]::new($false))
}
}
Copy-Item -LiteralPath (Join-Path $runtime 'SHA256SUMS.json') -Destination (Join-Path $target 'NEURAL-DOWNLOAD-SHA256SUMS.json') -Force

# Only this invocation's temporary directory can be removed.
$resolvedStage = (Resolve-Path -LiteralPath $stage).Path
if (-not [string]::Equals($resolvedStage, [IO.Path]::GetFullPath($stage), [StringComparison]::OrdinalIgnoreCase) -or
    -not $resolvedStage.StartsWith($target.TrimEnd('\') + '\.neural-setup-', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to clean a staging path outside the RPCS3 folder.'
}
Remove-Item -LiteralPath $resolvedStage -Recurse -Force
Write-Host 'Neural components installed. Existing INI/CFG settings were preserved.'
Write-Host 'In RPCS3: Config > Neural / ReShade > Neural / Feeder > choose a preset > Use preset > Save. Restart with Vulkan and your NVIDIA GPU selected.'
