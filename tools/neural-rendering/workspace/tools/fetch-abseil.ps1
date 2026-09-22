$ErrorActionPreference = 'Stop'
$taskWorkspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$archive = Join-Path $taskWorkspace 'downloads\abseil-20250512.1.zip'
$destination = Join-Path $PSScriptRoot 'abseil'
if (-not (Test-Path -LiteralPath $archive)) {
    & curl.exe --fail --location --retry 2 --output "$archive.part" 'https://github.com/abseil/abseil-cpp/archive/refs/tags/20250512.1.zip'
    if ($LASTEXITCODE -ne 0) { throw 'Failed to download Abseil.' }
    Move-Item -LiteralPath "$archive.part" -Destination $archive
}
Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
$source = Join-Path $destination 'abseil-cpp-20250512.1'
if (-not (Test-Path -LiteralPath (Join-Path $source 'CMakeLists.txt'))) { throw 'Unexpected Abseil archive layout.' }
# The build script passes the local source directory to CMake. Do not edit a
# generated CMakeCache.txt: its multiline comments have their own parser rules.
Get-FileHash -LiteralPath $archive -Algorithm SHA256
