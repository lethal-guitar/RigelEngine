# Rigel Engine [![Build Status](https://travis-ci.org/lethal-guitar/RigelEngine.svg?branch=master)](https://travis-ci.org/lethal-guitar/RigelEngine) [![Windows build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993. RigelEngine works as a drop-in replacement for the original executable: It reads the original data files and implements (mostly) the same behavior,
but runs natively on modern operating systems, and is written in modern C++ code with a completely new architecture under the hood. It is similar to
projects like [Omnispeak](https://davidgow.net/keen/omnispeak.html) or [Commander Genius](http://clonekeenplus.sourceforge.net), which do the same
thing for the Commander Keen series of games.

There was never any source code released for the original game, so this project
is based on reverse-engineering: A mix of reading the original executable's
disassembly and analyzing its behaviour using video captures of the game
running in DosBox.

I recently gave a presentation about the project at a local C++ meetup. You can find the slides [here](https://github.com/lethal-guitar/presentations).

## Current state

Here's a short video showcasing the project's current state:

<a href="http://www.youtube.com/watch?v=SO1M6gHvFzM"><img src="https://i.imgur.com/06btu7R.png" width="540"></img></a>

The project is still very much work in progress, but already partially playable. The first two levels of the 
Shareware episode are completely playable, not all of the following levels are completable due to missing
game mechanics, but some of the enemies found in those levels are already
functional. Overall, over 50 % of the functionality found in the original Shareware episode is reimplemented in
Rigel Engine so far. The [issue list](https://github.com/lethal-guitar/RigelEngine/issues) should give a 
comprehensive overview of what's still missing.

## Building and running

### Linux build quick start guide

If you're on Linux and running a recent enough Ubuntu/Debian-like distro<sup>[1](#foot-note-linux)</sup>,
here's how to quickly get the project up and running. Instructions for
[OS X](#mac-build-instructions) and [Windows](#windows-build-instructions) can
be found further down.

```bash
# Install all external dependencies, as well as the CMake build system:
sudo apt-get install cmake libboost-all-dev libsdl2-dev libsdl2-mixer-dev

# Clone the repo and initialize submodules:
git clone git@github.com:lethal-guitar/RigelEngine.git
cd RigelEngine
git submodule update --init --recursive

# Configure and build:
mkdir build
cd build
cmake ..
make

# NOTE:  Pass -j<NUM_PROCESSES> to 'make' in order to get multi-core
# compilation, '8' is a good number for a 4-core machine with hyperthreading

# Now run it!
./src/RigelEngine <PATH_TO_YOUR_GAME_FILES>
```

### Acquiring the game data

In order to run RigelEngine, the game data from the original game is required. Both the shareware version and the registered version work.
The game is currently not being sold anymore after the rights to the whole Duke Nukem franchise went to Gearbox, but you can still download the freely available shareware version from [the old 3D Realms site](http://legacy.3drealms.com/duke2/) - look for a download link for the file `4duke.zip`. You can also find the same file on various websites if you Google for "Duke Nukem 2 shareware".

The download contains an installer which only runs on MS-DOS, but you don't need that - you can simply rename the file `DN2SW10.SHR` (also part of the download) to `.zip` and open it using your favorite archive manager. After that, you can point RigelEngine to the directory where you extracted the files, and it should work.

If you already have a copy of the game, you can also point RigelEngine to that existing installation.

The only files actually required for RigelEngine are:

* `NUKEM2.CMP` (the main data file)
* `NUKEM2.F1`, `.F2`, `.F3` etc. up to `.F5` (intro movie files)

Currently, the game will abort if the intro movies are missing, but they aren't mandatory for gameplay, and I'm planning to make them optional in the future.


### Jumping to specific levels, command line options

Since most levels cannot be completed at the moment, only the first level of each episode is accessible
from the main menu. In order to try different levels, you can use the `-l` command line option:

```bash
# Jump to 6th level of first episode
./src/RigelEngine <PATH_TO_YOUR_GAME_FILES> -l L6
```

This will skip the intro movies and main menu, and jump straight to the specified level at Medium difficulty.

Other command line options are:

* `-s`: skip intro movies, go straight to main menu
* `--no-music`: don't play music
* `-h`/`--help`: show all command line options

### Detailed build pre-requisites and dependencies

To build from source, a C++ 14 compatible compiler is required. The project has been
built successfully on the following compilers:

* Microsoft Visual Studio 2015 (Update 2 or newer)
* gcc 5.4.0
* clang 3.9

Slightly older versions of gcc/clang might also work, but I haven't tried that.

The project depends on the following libraries:

* SDL >= 2.0.4
* SDL\_mixer >= 2.0.1
* Boost >= 1.55

The following further dependencies are already provided as submodules or source
code (in the `3rd_party` directory):

* [entityx](https://github.com/alecthomas/entityx) Entity-Component-System framework, v. 1.1.2
* Speex Resampler (taken from [libspeex](http://www.speex.org/))
* DBOPL AdLib emulator (taken from [DosBox](http://www.dosbox.com/))
* [Catch](https://github.com/philsquared/Catch) testing framework
* [atria](https://github.com/Ableton/atria) C++ utilities

### <a name="mac-build-instructions">OS X builds</a>

Building on OS X works almost exactly like the Linux build, except for getting
the dependencies. If you have homebrew, you can get them using the following:

```bash
brew install cmake sdl2 sdl2_mixer boost
```

### <a name="windows-build-instructions">Windows builds</a>

To build on Windows, you'll need to install CMake and provide binaries for the
external dependencies listed above. You can get them using the following links:

* [CMake 3.6.2](https://cmake.org/files/v3.6/cmake-3.6.2-win64-x64.zip)
* [SDL2 2.0.4](https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip)
* [SDL2 mixer 2.0.1](https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.1-VC.zip)
* [Boost 1.61](https://sourceforge.net/projects/boost/files/boost-binaries/1.61.0/boost_1_61_0-msvc-14.0-64.exe/download)

In order for CMake (or cmake-gui) to automatically find all dependencies, you
can set the following environment variables prior to running CMake:

```bash
# Assuming you've used the Boost installer linked above, and installed to BOOST_LOCATION
BOOST_ROOT=<BOOST_LOCATION>
BOOST_LIBRARYDIR=<BOOST_LOCATION>/lib64-msvc-14.0

# These should point to the respective root directory of the unzipped packages linked above
SDL2DIR=<SDL2_LOCATION>
SDL2MIXERDIR=<SDL2_MIXER_LOCATION>
```

If you're using git-bash on Windows, an easy way to do so is by adding the
corresponding `export` commands in your`.bashrc`.

_Note_: All of the above would need slight adjustments in order to create a
32-bit build. Also I haven't tried building 32-bit so far, so it's possible
that some additional changes would be necessary.

***

<a name="foot-note-linux">[1]</a>: I'm using Linux Mint 18, based on Ubuntu 16.04 Xenial Xerus
