# Rigel Engine [![Windows build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master) [![CI](https://github.com/lethal-guitar/RigelEngine/workflows/CI/badge.svg)](https://github.com/lethal-guitar/RigelEngine/actions?query=event%3Apush+branch%3Amaster) [![Discord](https://img.shields.io/discord/798281609437642752?color=7289DA&label=Discord&logo=discord&logoColor=white)](https://discord.gg/QKYB6u4Kew) [![GitHub All Releases](https://img.shields.io/github/downloads/lethal-guitar/RigelEngine/total.svg?style=flat)](https://github.com/lethal-guitar/RigelEngine/releases)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993 for MS-DOS. RigelEngine works as a drop-in replacement for the original executable: It reads the game's data files and plays just like the original,
but runs natively on modern operating systems and is written in modern C++ code.

Compared to running the original in an emulator, RigelEngine offers many modern enhancements and usability improvements, including:

* wide-screen mode
* smooth scrolling & movement mode with increased frame rate
* quick saving
* improved game controller support
* extended modding features

<img src="https://nikolai-wuttke.de/RigelEngine/assets/screenshot.png" width="500"/>

There was never any source code released for the original game, so this project
is based on reverse engineering (specifically, disassembly of the original binaries).
See [Duke2Reconstructed](https://github.com/lethal-guitar/Duke2Reconstructed) for a complete reconstruction of the original code.

Try the [web version](https://rigelengine.nikolai-wuttke.de)! (compiled to wasm via Emscripten)

Read the [F.A.Q.](https://github.com/lethal-guitar/RigelEngine/wiki/FAQ)

Watch a [showcase video (outdated)](http://www.youtube.com/watch?v=U7-eotm8Xoo)

## Current state

RigelEngine is stable and fairly complete in terms of feature set.
All four episodes of the game (shareware and registered version) are fully playable and on par with the original game.

I currently don't have any plans for significant future development - consider the project to be in "maintenance" mode.
This means I'll try my best to fix major problems/bugs that come up and keep CI running, but I won't add any new features. 

### Supported platforms

RigelEngine runs on Windows, Linux, and Mac OS X.

It's fairly easy to [install on Steam Deck](https://github.com/lethal-guitar/RigelEngine/wiki/Steam-Deck-setup), too.

The Linux version also runs well on small single-board computers (SBCs) like the Raspberry Pi and Odroid Go Advance.
See [Running on Raspberry Pi and Odroid Go Advance](https://github.com/lethal-guitar/RigelEngine/wiki/Running-on-Raspberry-Pi-and-Odroid-Go-Advance).

There's also a (3rd party) [PS Vita port](https://vitadb.rinnegatamante.it/#/info/988).

### System requirements

RigelEngine is not very demanding, but it does require OpenGL-capable graphics hardware.
Either OpenGL 3.0 or OpenGL ES 2.0 can be chosen at compile time.

Any Nvidia or AMD graphics card from 2007 or later should run the game without problems.
Intel integrated GPUs only added OpenGL 3 support in 2011, however.
On Linux, using GL ES can be an option for those older Intel GPUs.

See [Supported Graphics cards](https://github.com/lethal-guitar/RigelEngine/wiki/Supported-graphics-cards-(GPUs)) for more info.

Aside from the graphics card, you don't need much.
The game needs less than 64 MB of RAM,
and runs fine on a single-core ARMv6 CPU clocked at 700 MHz (Raspberry Pi 1).

## Contributing

Contributions to RigelEngine are very welcome! Please have a look at the [contribution guide](CONTRIBUTING.md) before making a PR.

There is a growing body of documentation on the Wiki, to help with getting into the code base. A good place to start is [Architecture Overview](https://github.com/lethal-guitar/RigelEngine/wiki/Architecture-overview)

If you are looking for some easy tasks to get started, take a look at issues labeled [good first issue](https://github.com/lethal-guitar/RigelEngine/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22).

## Running RigelEngine

In order to run RigelEngine, the game data from the original game is required. Both the shareware version and the registered version work.
When launching RigelEngine for the first time, it will show a file browser UI and ask you to select the location of your Duke Nukem II installation.
The chosen path will be stored in the game's user profile, so that you don't have to select it again next time.

It's also possible to pass the path to the game files as argument on the command line, which can be handy during development.

### Acquiring the game data

You can download the shareware version for free from [archive.org](https://archive.org/download/msdos_DUKE2_shareware/DUKE2.zip).
The full version (aka registered version) can be bought on [Zoom Platform](https://www.zoom-platform.com/product/duke-nukem-2),
a store which still has a license to sell the game (unlike other digital stores like Steam and Gog).

If you already have a copy of the game, you can also point RigelEngine to that existing installation.

The only files actually required for RigelEngine are:

* `NUKEM2.CMP` (the main data file)
* `NUKEM2.F1`, `.F2`, `.F3` etc. up to `.F5` (intro movie files)

The intro movies aren't mandatory, RigelEngine simply skips movie playback if the files aren't found.
They are still part of the experience though, so I do recommend including them when copying the game files somewhere.

If there are existing saved games, high score lists, or settings found in the game files,
RigelEngine imports them into its own user profile when running for the first time.

### Command line options, debugging tools, more info

You can find more info that didn't quite fit in this README over on [the Wiki](https://github.com/lethal-guitar/RigelEngine/wiki). For example, you'll find info on how to activate the built-in debugging tools, a list of bugs in the original version that have been fixed in Rigel Engine, etc.

## Pre-built binaries

Pre-built binaries are provided with each [Release](https://github.com/lethal-guitar/RigelEngine/releases).
As of version 0.8.0, this includes Windows (x64),
Mac OS (x64),
and `deb` packages for Debian/Ubuntu/Mint Linux distros (also x64).

A [Flatpak](https://flathub.org/apps/details/io.github.lethal_guitar.RigelEngine) is also available.

Also see [third-party Linux builds](https://github.com/lethal-guitar/RigelEngine/wiki/Third-party-Linux-builds)
for a list of other Linux packages/builds provided by distros and other projects.

## Building from source

See [BUILDING.md](BUILDING.md) for detailed instructions for each platform.
