RPCS3 Neural / ReShade — unofficial experimental fork
====================================================

[Download Windows x64 release](https://github.com/MarcPique/RPCS3-with-Neural-Rendering/releases/tag/v0.2.2-neural) · [Setup and controls](NEURAL_RENDERING.md) · [Validation](docs/neural-rendering/VALIDACION.md)

This fork adds a **Neural / ReShade** settings tab, local Vulkan integration, sliders with displayed values for every numeric Neural / Feeder control, and four quick presets: Suave, Equilibrado, Detalle and Cinematográfico. The component selection follows [DLSS5oneclick](https://github.com/faisalkindi/DLSS5oneclick). This is community integration through ReShade/Feeder, not an official NVIDIA DLSS integration or an official RPCS3 release.

Download **`Extraer-RPCS3-Neural-0.2.2-win64.exe`** (self-extracting package) or extract **`RPCS3-Neural-0.2.2-public-win64.zip`** completely. Both contain RPCS3 and its DLLs, including OpenCV and Qt. Open `rpcs3.exe` inside the extracted folder; do not move it away from its DLLs or run it from inside a ZIP.

In **Config → Neural / ReShade**, click **Descargar / reparar componentes**, or enable the ReShade checkbox to download missing components automatically. Progress, cancellation and errors are shown in RPCS3. No terminal is needed. Select Vulkan and your compatible NVIDIA GPU, choose a quick preset if desired, save and restart. The in-game ReShade menu is blocked for keyboard and gamepad; configure effects in RPCS3 settings. Disabling the integration requires saving and restarting RPCS3. Activation is off by default; the download preserves pending settings until you save.

The public archive contains the emulator, its redistributable dependencies, licenses and setup scripts. It excludes neural models/add-ons and LumeniteFX; those are downloaded locally because their redistribution is restricted or unverified. Requires Windows x64 and the Microsoft Visual C++ x64 runtime compatible with MSVC 14.51 or later.

**Experimental:** configuration/UI tests, Vulkan device creation, add-on loading and a separate 300-frame neural diagnostic passed on an RTX 4090. Saw II reached gameplay with successful neural feature-18 evaluations at 3840×2160. Visual comparisons across presets, depth/motion quality and prolonged stability remain unvalidated; users should evaluate these themselves. [Build tools and pinned dependencies](tools/neural-rendering/README.md) are included; clone with submodules when rebuilding.

RPCS3 (upstream project)
=====

[![GitHub Actions](https://img.shields.io/github/actions/workflow/status/RPCS3/rpcs3/rpcs3.yml?branch=master&logo=github&label=Actions)](https://github.com/RPCS3/rpcs3/actions/workflows/rpcs3.yml)
[![RPCS3 Discord Server](https://img.shields.io/discord/272035812277878785?color=5865F2&label=RPCS3%20Discord&logo=discord&logoColor=white)](https://discord.gg/rpcs3)

The world's first free and open-source PlayStation 3 emulator/debugger, written in C++ for Windows, Linux, macOS and FreeBSD.

You can find some basic information on our [**website**](https://rpcs3.net/). Game info is being populated on the [**Wiki**](https://wiki.rpcs3.net/).
For discussion about this emulator, PS3 emulation, and game compatibility reports, please visit our [**forums**](https://forums.rpcs3.net) and our [**Discord server**](https://discord.gg/RPCS3).

[**Support the Lead Developers on Patreon**](https://rpcs3.net/patreon)

## Contributing

If you want to help the project but do not code, the best way to help out is to test games and make bug reports. See:
* [Quickstart](https://rpcs3.net/quickstart)

If you want to contribute as a developer, please take a look at the following pages:

* [Coding Style](https://github.com/RPCS3/rpcs3/wiki/Coding-Style)
* [Developer Information](https://github.com/RPCS3/rpcs3/wiki/Developer-Information)

You should also contact any of the developers in the forums or in the Discord server to learn more about the current state of the emulator.

### AI Use

Use of AI tools for research and reverse engineering purposes is permitted. However, contributors are expected to fully own and understand all code they submit. Any communication with the team — including code, code comments, and GitHub comments — must come from the human contributor, not an AI agent acting autonomously.

We have unfortunately seen a rise in untested and unverified AI-generated slop being submitted to this project. This wastes maintainer time and, in worse cases, such changes get merged and break functionality for all users. Repeated violations will result in a ban from the repository. Please be respectful of everyone's time.

**Pull requests opened by AI agents or automated tools must include a disclosure in the PR description** stating the scope of AI involvement — which parts were AI-generated and what human testing or review was performed prior to submission. PRs that omit this disclosure may be closed without review.

If you are unsure about your work, open a discussion issue to talk it through with the team, or reach out to a maintainer on [Discord](https://discord.gg/RPCS3).

## Building

See [BUILDING.md](BUILDING.md) for more information about how to setup an environment to build RPCS3.

## Running

Check our friendly [quickstart](https://rpcs3.net/quickstart) guide to make sure your computer meets the minimum system requirements to run RPCS3.

Don't forget to have your graphics driver up to date and to install the [Visual C++ Redistributable Packages for Visual Studio 2022](https://aka.ms/vs/17/release/VC_redist.x64.exe) if you are a Windows user.

## License

Most files are licensed under the terms of GNU GPL-2.0-only License; see LICENSE file for details. Some files may be licensed differently; check appropriate file headers for details.
