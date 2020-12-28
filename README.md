# Rigel Engine [![Windows build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master) [![CI](https://github.com/lethal-guitar/RigelEngine/workflows/CI/badge.svg)](https://github.com/lethal-guitar/RigelEngine/actions?query=event%3Apush+branch%3Amaster) [![Join the chat at https://gitter.im/RigelEngine/community](https://badges.gitter.im/RigelEngine/community.svg)](https://gitter.im/RigelEngine/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![GitHub All Releases](https://img.shields.io/github/downloads/lethal-guitar/RigelEngine/total.svg?style=flat)](https://github.com/lethal-guitar/RigelEngine/releases)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993. RigelEngine works as a drop-in replacement for the original executable: It reads the original data files and offers the same experience (with some improvements),
but runs natively on modern operating systems, and is written in modern C++ code with a completely new architecture under the hood. It is similar to
projects like [Omnispeak](https://davidgow.net/keen/omnispeak.html) or [Commander Genius](http://clonekeenplus.sourceforge.net), which do the same
thing for the Commander Keen series of games.

There was never any source code released for the original game, so this project
is based on reverse-engineering the original executable's assembly code. Video captures from DosBox are used to verify re-implemented behavior.
See [my blog post](https://lethalguitar.wordpress.com/2019/05/28/re-implementing-an-old-dos-game-in-c-17/) to learn more about the process.

Here's a video showcasing the project:

<a href="http://www.youtube.com/watch?v=U7-eotm8Xoo"><img src="https://i.imgur.com/06btu7R.png" width="540"></img></a>

## Current state

Gameplay-wise, RigelEngine is feature-complete: All four episodes of the game (shareware and registered version) are fully playable and on par with the original game.

The project overall is far from finished, though. There are still some pieces missing to reach full parity with the original game ([a few visual effects](https://github.com/lethal-guitar/RigelEngine/milestone/5), [demo playback](https://github.com/lethal-guitar/RigelEngine/milestone/13)). On top of that, more [modern enhancements](https://github.com/lethal-guitar/RigelEngine/labels/enhancement) and [usability improvements](https://github.com/lethal-guitar/RigelEngine/labels/usability) are planned.

### Supported platforms

RigelEngine runs on Windows, Linux, and Mac OS X.

The Linux version also runs well on small single-board computers (SBCs) like the Raspberry Pi and Odroid Go Advance.
See [Running on Raspberry Pi and Odroid Go Advance](https://github.com/lethal-guitar/RigelEngine/wiki/Running-on-Raspberry-Pi-and-Odroid-Go-Advance).

Android and iOS versions might happen someday, but there are no concrete plans at the moment.

### System requirements

RigelEngine is not very demanding, but it does require OpenGL-capable graphics hardware.
Either OpenGL 3.0 or OpenGL ES 2.0 can be used, depending on what's chosen at compile time.
To build in GL ES mode, pass `-DUSE_GL_ES=ON` to CMake.

Any Nvidia or AMD graphics card from 2007 or later should run the game without problems.
Intel integrated GPUs only added OpenGL 3 support in 2011, however.
On Linux, using GL ES can still be an option for older Intel GPUs.

See [Supported Graphics cards](https://github.com/lethal-guitar/RigelEngine/wiki/Supported-graphics-cards-(GPUs)) for more info.

### Differences to the original Duke Nukem II

See [list of differences](https://github.com/lethal-guitar/RigelEngine/wiki#differences-to-the-original-duke-nukem-ii-executable).

### What about Duke Nukem 1 and Cosmo's Cosmic Adventure?

RigelEngine focuses exlusively on Duke Nukem II. For Cosmo and Duke 1, there are already other projects: [Cosmo-Engine](https://github.com/yuv422/cosmo-engine) and [ReDuke](http://k1n9duk3.shikadi.net/reduke.html). Aside from that, Duke Nukem 1 is using a completely different engine, so supporting it with this same project doesn't really make sense. This is a bit different for Cosmo, since its engine actually served as basis for Duke Nukem II's engine. Still, the two are different enough that supporting both games with one engine is not really feasible or useful.

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

The full version of the game (aka registered version) is not available currently, but you can still download the freely available shareware version, e.g. from [archive.org](https://archive.org/download/msdos_DUKE2_shareware/DUKE2.zip).

If you already have a copy of the game, you can also point RigelEngine to that existing installation.

The only files actually required for RigelEngine are:

* `NUKEM2.CMP` (the main data file)
* `NUKEM2.F1`, `.F2`, `.F3` etc. up to `.F5` (intro movie files)

Currently, the game will abort if the intro movies are missing, but they aren't mandatory for gameplay, and I'm planning to make them optional in the future.

If there are existing saved games, high score lists, or settings found in the game files,
RigelEngine imports them into its own user profile when running for the first time.

### Command line options, debugging tools, more info

You can find more info that didn't quite fit in this README over on [the Wiki](https://github.com/lethal-guitar/RigelEngine/wiki). For example, you'll find info on how to activate the built-in debugging tools, a list of bugs in the original version that have been fixed in Rigel Engine, etc.

## Getting binaries

Pre-built binaries for Windows 64-bit are provided with each [Release](https://github.com/lethal-guitar/RigelEngine/releases). You can also grab a build of the latest `master` branch by going to [AppVeyor](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master), clicking on "Configuration: Release", and then clicking on "Artifacts".

Thanks to [@mnhauke](https://github.com/mnhauke), there is now also a [Linux package for OpenSUSE Tumbleweed](https://software.opensuse.org/package/RigelEngine).

I'm planning to provide binaries for OS X, Ubuntu/Debian, and Raspberry Pi in the future, but right now, you need to build the project yourself on these platforms.

## Building from source

See [BUILDING.md](BUILDING.md)
