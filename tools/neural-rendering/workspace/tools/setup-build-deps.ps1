[CmdletBinding()]
param(
    [switch]$QtOnly,
    [switch]$SkipSubmodules,
    [switch]$KeepArchives
)

# Portable dependencies only. No installers, registry writes or system changes.
# Versions and Qt archive names match rpcs3/.github/workflows/rpcs3.yml.
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$repo = Join-Path $workspace 'rpcs3'
$downloads = Join-Path $workspace 'downloads'
$qt = Join-Path $workspace 'tools\qt\6.11.2\msvc2022_64'
$vulkan = Join-Path $workspace 'tools\vulkan\1.4.341.0'
$llvm = Join-Path $repo 'build\lib_ext\Release-x64'
$sevenZip = 'C:\Program Files\7-Zip\7z.exe'
$vs = 'C:\Program Files\Microsoft Visual Studio\18\Community'
$compilerBin = Join-Path $vs 'VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64'
$records = [Collections.Generic.List[object]]::new()

function Assert-WorkspacePath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (-not $full.StartsWith($workspace + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing operation outside workspace: $full"
    }
    return $full
}

function Assert-FreeSpace([long]$AdditionalBytes = 0) {
    $drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($workspace))
    if ($drive.AvailableFreeSpace -lt (3GB + $AdditionalBytes)) {
        throw ('Insufficient space: {0:N2} GiB free; preserve 3 GiB plus {1:N2} GiB for next operation.' -f ($drive.AvailableFreeSpace / 1GB), ($AdditionalBytes / 1GB))
    }
}

function Get-Download([string]$Url, [string]$Name) {
    $target = Assert-WorkspacePath (Join-Path $downloads $Name)
    if (-not (Test-Path -LiteralPath $target)) {
        Assert-FreeSpace 1GB
        $partial = Assert-WorkspacePath ($target + '.part')
        Write-Host "Downloading $Url"
        & curl.exe --fail --location --retry 2 --connect-timeout 20 --max-time 1800 --output $partial $Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed ($LASTEXITCODE): $Url" }
        Move-Item -LiteralPath $partial -Destination $target -Force
    }
    return $target
}

function Expand-VerifiedArchive([string]$Archive, [string]$Destination) {
    $Destination = Assert-WorkspacePath $Destination
    $listing = & $sevenZip l -slt $Archive
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect archive $Archive" }
    $totalBytes = 0L
    foreach ($line in $listing) {
        if ($line -match '^Size = (\d+)$') { $totalBytes += [long]$Matches[1] }
    }
    Assert-FreeSpace $totalBytes
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    & $sevenZip x -y -bsp0 -bso0 "-o$Destination" $Archive
    if ($LASTEXITCODE -ne 0) { throw "Archive extraction failed ($LASTEXITCODE): $Archive" }
    if (-not $KeepArchives) {
        $safeArchive = Assert-WorkspacePath $Archive
        if (-not $safeArchive.StartsWith($downloads + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Archive is outside downloads.' }
        Remove-Item -LiteralPath $safeArchive -Force
    }
}

function Install-CheckedArchive([string]$Url, [string]$Name, [string]$Destination, [string]$Algorithm, [string]$ChecksumUrl) {
    $marker = Assert-WorkspacePath (Join-Path $downloads ($Name + '.installed.json'))
    if (Test-Path -LiteralPath $marker) {
        $records.Add((Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json))
        Write-Host "Already extracted: $Name"
        return
    }
    $checksumFile = Get-Download $ChecksumUrl ($Name + '.' + $Algorithm.ToLowerInvariant())
    $expected = ((Get-Content -LiteralPath $checksumFile -Raw).Trim() -split '\s+')[0]
    if ($expected -notmatch '^[0-9a-fA-F]{40}$|^[0-9a-fA-F]{64}$') { throw "Invalid checksum: $ChecksumUrl" }
    $archive = Get-Download $Url $Name
    $actual = (Get-FileHash -LiteralPath $archive -Algorithm $Algorithm).Hash
    if ($actual -ne $expected) { throw "Checksum mismatch: $Name" }
    $record = [ordered]@{ url = $Url; name = $Name; algorithm = $Algorithm; hash = $actual; bytes = (Get-Item -LiteralPath $archive).Length; destination = $Destination }
    Expand-VerifiedArchive $archive $Destination
    $record | ConvertTo-Json | Set-Content -LiteralPath $marker -Encoding utf8
    $records.Add($record)
}

if (-not (Test-Path -LiteralPath $sevenZip)) { throw '7-Zip is required.' }
if (-not (Test-Path -LiteralPath (Join-Path $compilerBin 'lib.exe'))) { throw 'MSVC 2026 compiler tools missing.' }
New-Item -ItemType Directory -Path $downloads,$qt,$vulkan,$llvm -Force | Out-Null
Assert-FreeSpace

$qtBaseUrl = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6112/qt6_6112_msvc2022_64'
$qtPrefix = '6.11.2-0-202608131017'
$qtSuffix = '-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z'
$modules = @('qtbase')
if (-not $QtOnly) { $modules += @('qtsvg','qtmultimedia','qttools','qtdeclarative','qttranslations') }
foreach ($module in $modules) {
    $component = if ($module -eq 'qtmultimedia') { 'qt.qt6.6112.addons.qtmultimedia.win64_msvc2022_64' } else { 'qt.qt6.6112.win64_msvc2022_64' }
    $name = $qtPrefix + $module + $qtSuffix
    $url = "$qtBaseUrl/$component/$name"
    Install-CheckedArchive $url $name $qt 'SHA1' ($url + '.sha1')
    if ($module -eq 'qtbase') {
        $core = @(Get-ChildItem -LiteralPath $qt -Filter 'Qt6Core.dll' -Recurse -File)
        if ($core.Count -ne 1) { throw 'QtCore installation path is ambiguous or missing.' }
        $actualQt = Split-Path (Split-Path $core[0].FullName -Parent) -Parent
        $qtConf = Join-Path $actualQt 'bin\qt.conf'
        if (-not (Test-Path -LiteralPath $qtConf)) {
            @('[Paths]', 'Prefix=..') | Set-Content -LiteralPath $qtConf -Encoding ascii
        }
        Write-Host "QTBASE_READY=$actualQt"
        $actualQt | Set-Content -LiteralPath (Join-Path $downloads 'qt-ready.txt') -Encoding utf8
    }
}

if (-not $QtOnly) {
    $llvmUrl = 'https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-22.1.8/llvmlibs_mt.7z'
    Install-CheckedArchive $llvmUrl 'llvmlibs_mt.7z' $llvm 'SHA256' ($llvmUrl + '.sha256')
    if (-not (Test-Path -LiteralPath (Join-Path $llvm 'llvm_build\include\llvm\IR\Module.h'))) { throw 'Prebuilt LLVM headers missing.' }
    if (-not (Test-Path -LiteralPath (Join-Path $llvm 'llvm_build\lib\LLVMCore.lib'))) { throw 'Prebuilt LLVMCore library missing.' }

    # Headers only; linking uses the Vulkan loader already supplied by the GPU driver.
    $headersUrl = 'https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/vulkan-sdk-1.4.341.0.zip'
    $headersMarker = Join-Path $downloads 'vulkan-headers.installed.json'
    if (-not (Test-Path -LiteralPath $headersMarker)) {
        $headersArchive = Get-Download $headersUrl 'vulkan-headers-1.4.341.0.zip'
        $headersRecord = [ordered]@{ url = $headersUrl; sha256 = (Get-FileHash -LiteralPath $headersArchive -Algorithm SHA256).Hash }
        Expand-VerifiedArchive $headersArchive $vulkan
        $headerRoots = @(Get-ChildItem -LiteralPath $vulkan -Directory -Filter 'Vulkan-Headers-*')
        if ($headerRoots.Count -ne 1) { throw 'Vulkan-Headers archive structure unexpected.' }
        $headersInclude = Assert-WorkspacePath (Join-Path $headerRoots[0].FullName 'include')
        $destinationInclude = Assert-WorkspacePath (Join-Path $vulkan 'Include')
        if (-not (Test-Path -LiteralPath $destinationInclude)) { Move-Item -LiteralPath $headersInclude -Destination $destinationInclude }
        $headersRecord | ConvertTo-Json | Set-Content -LiteralPath $headersMarker -Encoding utf8
    }
    $loader = Join-Path $env:SystemRoot 'System32\vulkan-1.dll'
    if (-not (Test-Path -LiteralPath $loader)) { throw 'Install a GPU driver with Vulkan support before building.' }
    $exports = & (Join-Path $compilerBin 'dumpbin.exe') /nologo /exports $loader
    if ($LASTEXITCODE -ne 0) { throw 'Could not inspect installed Vulkan loader exports.' }
    $exportNames = @($exports | ForEach-Object { if ($_ -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(vk\w+)') { $Matches[1] } })
    if ($exportNames.Count -lt 100) { throw 'Vulkan loader export list incomplete.' }
    $vulkanLib = Join-Path $vulkan 'Lib'
    New-Item -ItemType Directory -Path $vulkanLib -Force | Out-Null
    $def = Join-Path $vulkanLib 'vulkan-1.def'
    (@('LIBRARY vulkan-1.dll','EXPORTS') + $exportNames) | Set-Content -LiteralPath $def -Encoding ascii
    & (Join-Path $compilerBin 'lib.exe') /nologo /machine:x64 "/def:$def" "/out:$(Join-Path $vulkanLib 'vulkan-1.lib')"
    if ($LASTEXITCODE -ne 0) { throw 'Could not generate Vulkan import library.' }

    if (-not $SkipSubmodules) {
        $env:PATH = 'C:\Program Files\Git\usr\bin;C:\Program Files\Git\mingw64\bin;' + $env:PATH
        $paths = (Get-Content -LiteralPath (Join-Path $repo '.gitmodules') | Select-String '^\s*path = ' | ForEach-Object { ($_.Line -split ' = ',2)[1] }) | Where-Object { $_ -notmatch 'llvm|FAudio|feralinteractive' }
        Push-Location $repo
        try {
            & git -c core.longpaths=true submodule update --init --depth=1 --jobs=8 -- $paths
            if ($LASTEXITCODE -ne 0) { throw 'Submodule initialization failed.' }
        } finally { Pop-Location }
    }
}

$manifest = [ordered]@{ qt = $actualQt; vulkan = $vulkan; llvm = $llvm; visualStudio = $vs; compilerBin = $compilerBin; msbuild = (Join-Path $vs 'MSBuild\Current\Bin\amd64\MSBuild.exe'); dependencies = $records }
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $downloads 'build-environment.json') -Encoding utf8
Write-Host ('Dependencies ready. Qt: {0}' -f $actualQt)
Write-Host ('Free disk: {0:N2} GiB' -f ([IO.DriveInfo]::new([IO.Path]::GetPathRoot($workspace)).AvailableFreeSpace / 1GB))
