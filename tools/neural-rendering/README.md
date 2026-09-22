# Neural / ReShade release tools

`Setup-Neural.ps1` and `fetch-neural-runtime.ps1` ship together beside `rpcs3.exe` in the public Windows ZIP. The first script downloads the pinned runtime through the second, stages it, installs the managed files and preserves existing INI/CFG settings. Neither script runs downloaded code or modifies Vulkan's system registry. Download caches remain in `.neural-downloads` for reuse. `NEURAL-DOWNLOAD-SHA256SUMS.json` records the downloaded defaults; existing user configs may intentionally differ.

From a source checkout, supply `-RpcS3Directory` pointing to an extracted release. Windows PowerShell 5.1 or PowerShell 7 is sufficient for setup. No administrator rights are required for a writable portable folder.

The `workspace` directory preserves the scripts used to build this release. Copy its contents one level above a checkout named `rpcs3`, then run `tools/setup-build-deps.ps1`, `tools/fetch-abseil.ps1` and `build/build-rpcs3.ps1` using PowerShell 7. They pin Qt 6.11.2, LLVM 22.1.8 and Vulkan headers 1.4.341.0. Visual Studio 2026/MSVC 14.51 and 7-Zip must already be installed; adjust their paths in the scripts on another machine. For an incremental build, `-MinimumFreeGiB 2` reduces the initial disk-space check.

RPCS3's [BUILDING.md](../../BUILDING.md) remains the main cross-platform build guide. Windows x64 is the supported platform for this experimental neural runtime.

After committing the source, `package-public-release.ps1 -CleanStage <stage> -PackageDirectory <new-folder> -OutputDirectory <assets>` creates the public ZIP, standalone EXE and SHA256 list. Supply the clean emulator stage produced by `stage-rpcs3-release.ps1`, with notices from `collect-build-notices.ps1`. Include the Qt LGPL and LLVM license texts with those notices. The packager rejects neural runtimes and user configuration files, removes local build paths from archive provenance, and records the fork source commit.

To rebuild the release source, use `git clone --branch v0.2.0-neural --recurse-submodules https://github.com/MarcPique/rpcs3-neural.git rpcs3`. The automatic GitHub source ZIPs do not include submodule contents. Qt is dynamically linked; the bundled Qt libraries can be replaced with compatible builds. Upstream source for Qt 6.11.2 is available from https://download.qt.io/archive/qt/6.11/6.11.2/submodules/; the bundled source-provenance file identifies the LLVM and other upstream versions.

No neural DLLs, shaders, model files, firmware or games are committed to this repository. Components downloaded for local use retain their own licenses; downloading them does not grant permission to rehost them.
