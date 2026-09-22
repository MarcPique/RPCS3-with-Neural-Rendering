[CmdletBinding()]
param([int]$ParallelProjects = 2, [int]$CompilerProcesses = 8, [ValidateRange(2, 100)][int]$MinimumFreeGiB = 6)

$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestFile = Join-Path $workspace 'downloads\build-environment.json'
if (-not (Test-Path -LiteralPath $manifestFile)) { throw 'Run tools/setup-build-deps.ps1 first.' }
$deps = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$repo = Join-Path $workspace 'rpcs3'
$vsdevcmd = Join-Path $deps.visualStudio 'Common7\Tools\VsDevCmd.bat'
$ninjaDir = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
# The host can supply both Path and PATH; MSBuild rejects duplicate keys.
$savedPath = $env:PATH
Remove-Item Env:Path -ErrorAction SilentlyContinue
Remove-Item Env:PATH -ErrorAction SilentlyContinue
$env:Path = $savedPath
$cmdline = 'call "' + $vsdevcmd + '" -arch=amd64 -host_arch=amd64 >nul && set'
$environment = & $env:COMSPEC /d /c $cmdline
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio developer environment failed.' }
foreach ($line in $environment) {
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
$env:QTDIR = $deps.qt
$env:Qt6_ROOT = $deps.qt
$env:VULKAN_SDK = $deps.vulkan
$env:PATH = "$ninjaDir;C:\Program Files\CMake\bin;C:\Program Files\Git\usr\bin;C:\Program Files\Git\mingw64\bin;" + (Join-Path $deps.visualStudio 'Common7\Tools') + ';' + $env:PATH
$env:CMAKE_BUILD_PARALLEL_LEVEL = "$CompilerProcesses"
$env:CL_MPCount = "$CompilerProcesses"
$abseilSource = Join-Path $workspace 'tools\abseil\abseil-cpp-20250512.1'
if (Test-Path -LiteralPath (Join-Path $abseilSource 'CMakeLists.txt')) {
    $env:CCACHE_LAUNCH_ARGS = ($env:CCACHE_LAUNCH_ARGS + ' -DFETCHCONTENT_SOURCE_DIR_ABSL="' + $abseilSource.Replace('\','/') + '"').Trim()
}
$tempDir = Join-Path $workspace 'build\temp'
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
$env:TEMP = $tempDir
$env:TMP = $tempDir
foreach ($required in @((Join-Path $deps.qt 'bin\moc.exe'),(Join-Path $deps.vulkan 'Lib\vulkan-1.lib'),(Join-Path $deps.llvm 'llvm_build\lib\LLVMCore.lib'),(Join-Path $ninjaDir 'ninja.exe'))) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing build dependency: $required" }
}
$drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($workspace))
if ($drive.AvailableFreeSpace -lt ($MinimumFreeGiB * 1GB)) { throw "At least $MinimumFreeGiB GiB must be free before beginning this build." }
$log = Join-Path $workspace 'build\rpcs3-build.log'
$arguments = @('rpcs3.sln','/t:rpcs3','/nologo',"/m:$ParallelProjects",'/v:minimal','/clp:ErrorsOnly;Summary','/p:Configuration=Release','/p:Platform=x64','/p:PreferredToolArchitecture=x64','/p:UseMultiToolTask=true',"/p:MultiProcMaxCount=$CompilerProcesses",'/p:EnforceProcessCountAcrossBuilds=true',('/p:CustomAfterMicrosoftCommonTargets=' + (Join-Path $repo 'buildfiles\msvc\ci_only.targets')),'/fl',('/flp:LogFile=' + $log + ';Verbosity=normal;Encoding=UTF-8'))
Push-Location $repo
try {
    & $deps.msbuild @arguments
    $exitCode = $LASTEXITCODE
} finally { Pop-Location }
if ($exitCode -ne 0) { throw "RPCS3 build failed ($exitCode). See $log" }
Get-Item -LiteralPath (Join-Path $repo 'bin\rpcs3.exe') | Select-Object FullName,Length,LastWriteTime
