#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PackageDirectory, [Parameter(Mandatory)][string]$DumpbinPath)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PackageDirectory).Path
$missing = [Collections.Generic.List[string]]::new()
$count = 0
foreach ($binary in Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object { $_.Extension -in @('.dll','.exe') }) {
    $imports = & $DumpbinPath /nologo /dependents $binary.FullName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($binary.Name)" }
    foreach ($line in $imports) {
        $name = $line.Trim()
        if ($name -notmatch '^[A-Za-z0-9_.-]+\.dll$' -or $name -match '^(api-ms-|ext-ms-)') { continue }
        $paths = @((Join-Path $binary.DirectoryName $name), (Join-Path $root $name), (Join-Path "$env:SystemRoot/System32" $name))
        if (-not ($paths | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })) { $missing.Add("$($binary.Name) -> $name") }
    }
    $count++
}
if ($missing.Count) { throw ('Missing imported DLLs: ' + ($missing -join ', ')) }
Write-Host "PASS: imports of $count EXE/DLL files resolve within the package or Windows/System32 (including the installed Visual C++ runtime)."
