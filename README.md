# Rigel Engine [![Build Status](https://api.travis-ci.com/lethal-guitar/RigelEngine.svg?branch=master)](https://travis-ci.com/lethal-guitar/RigelEngine) [![Windows build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master) [![Join the chat at https://gitter.im/RigelEngine/community](https://badges.gitter.im/RigelEngine/community.svg)](https://gitter.im/RigelEngine/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993. RigelEngine works as a drop-in replacement for the original executable: It reads the original data files and offers the same experience (plus some improvements),
but runs natively on modern operating systems, and is written in modern C++ code with a completely new architecture under the hood. It is similar to
projects like [Omnispeak](https://davidgow.net/keen/omnispeak.html) or [Commander Genius](http://clonekeenplus.sourceforge.net), which do the same
thing for the Commander Keen series of games.

There was never any source code released for the original game, so this project
is based on reverse-engineering: A mix of reading assembly and analyzing video captures from DosBox.

Here's a video showcasing the project:

<a href="http://www.youtube.com/watch?v=Z3gCS5LvC2s"><img src="https://i.imgur.com/06btu7R.png" width="540"></img></a>

## Current state

RigelEngine implements all the game mechanics and enemies found in the original game's first episode, i.e. the shareware episode. The intro movie, story sequence and most of the menu system are implemented as well. It's possible to load and save the game, and the high score lists work (saved games and high scores from the original game will be imported into RigelEngine's user profile when launching it for the first time). Therefore, the shareware episode is fully playable with RigelEngine.

The levels from the registered version (i.e. episodes 2, 3 and 4) can also be loaded, and they should mostly work, but not all of the enemies found in those levels are functional yet. In addition, Duke's space ship is not implemented yet, which means the levels where it is required can't be completed yet.

Some other features that still need to be implemented:

* Demo playback
* Enemy radar in game
* Options menu/configuring the game

Plus, it would be nice to make launching the game easier in the future, e.g. by having some kind of launcher application or setup program.

## Running RigelEngine

In order to run RigelEngine, the game data from the original game is required. Both the shareware version and the registered version work. To make RigelEngine find the game data, you can either:

a) copy the RigelEngine executable (and accompanying DLLs) into the directory containing the game data, and launch `RigelEngine.exe` instead of `NUKEM2.EXE`. This is the recommended way to run it on Windows.
b) pass the path to the game data as a command line argument to RigelEngine. This is the recommended way to run it on Linux/OS X.

For example, let's say you have your copy of Duke Nukem II in `/home/niko/Duke2`, and a build of RigelEngine in `/home/niko/RigelEngine/build`. You would then start the game as follows:

```
cd /home/niko/RigelEngine/build
./src/RigelEngine /home/niko/Duke2
```

### Acquiring the game data

The full version of the game (aka registered version) is not available currently, but you can still download the freely available shareware version from [the old 3D Realms site](http://legacy.3drealms.com/duke2/) - look for a download link for the file `4duke.zip`. You can also find the same file on various websites if you Google for "Duke Nukem 2 shareware".

Note that on macOS you might need to unzip from the terminal - `unzip 4duke.zip`, since the built-in unarchiver seems to dislike the shareware download.

The download contains an installer which only runs on MS-DOS, but you don't need that - you can simply rename the file `DN2SW10.SHR` (also part of the download) to `.zip` and open it using your favorite archive manager. After that, you can point RigelEngine to the directory where you extracted the files, and it should work.

If you already have a copy of the game, you can also point RigelEngine to that existing installation.

The only files actually required for RigelEngine are:

* `NUKEM2.CMP` (the main data file)
* `NUKEM2.F1`, `.F2`, `.F3` etc. up to `.F5` (intro movie files)

Currently, the game will abort if the intro movies are missing, but they aren't mandatory for gameplay, and I'm planning to make them optional in the future.


### Command line options

The most important command line options are:

* `-l`: jump to a specific level. E.g. `-l L5` to play the 5th level of episode 1.
* `-s`: skip intro movies, go straight to main menu
* `--no-music`: don't play music
* `-h`/`--help`: show all command line options

## Getting binaries

Pre-built binaries are provided for Windows. You can grab them from the [Releases tab](https://github.com/lethal-guitar/RigelEngine/releases). Alternatively, you can grab a build of the latest `master` branch by going to [AppVeyor](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master), clicking on "Configuration: Release", and then clicking on "Artifacts".

I'm planning to also provide binaries for OS X and Linux in the future. But right now, you have to build the project yourself on these platforms.

## Building from source

### Get the sources

First of all, get the sources:

```bash
# Clone the repo and initialize submodules:
git clone git@github.com:lethal-guitar/RigelEngine.git
cd RigelEngine
git submodule update --init --recursive
``` 

### A note about warnings as errors

By default, warnings during compilation are treated as errors. This behavior
can be changed by passing `-DWARNINGS_AS_ERRORS=OFF` to CMake when configuring.
If you plan to work on RigelEngine, I'd recommend leaving this on, as you might
otherwise have your build fail on CI despite it building successfully locally.

On the other hand, if you only want to use RigelEngine, but there are no
pre-built binaries for your platform, disabling warnings as errors is
recommended.

### Linux build quick start guide

If you're on Linux and running a recent enough Ubuntu/Debian-like distro<sup>[1](#foot-note-linux)</sup>,
here's how to quickly get the project up and running. Instructions for
[OS X](#mac-build-instructions) and [Windows](#windows-build-instructions) can
be found further down.

```bash
# Install all external dependencies, as well as the CMake build system:
sudo apt-get install cmake libboost-all-dev libsdl2-dev libsdl2-mixer-dev

# Configure and build:
mkdir build
cd build
cmake .. -DWARNINGS_AS_ERRORS=OFF
make

# NOTE:  Pass -j<NUM_PROCESSES> to 'make' in order to get multi-core
# compilation, '8' is a good number for a 4-core machine with hyperthreading
#
# If you plan to develop RigelEngine, I recommend dropping the
# -DWARNINGS_AS_ERRORS part - see the note about warnings as errors above.

# Now run it!
./src/RigelEngine <PATH_TO_YOUR_GAME_FILES>
```

### Detailed build pre-requisites and dependencies

To build from source, a C++ 17 compatible compiler is required. The project has been
built successfully on the following compilers:

* Microsoft Visual Studio 2017 (version 15.9.4 or newer)
* gcc 8.1.0
* clang 7.0.0

Slightly older versions of gcc/clang might also work, but I haven't tried that.

The project depends on the following libraries:

* SDL >= 2.0.4
* SDL\_mixer >= 2.0.1
* Boost >= 1.67

The following further dependencies are already provided as submodules or source
code (in the `3rd_party` directory):

* [entityx](https://github.com/alecthomas/entityx) Entity-Component-System framework, v. 1.1.2
* Speex Resampler (taken from [libspeex](http://www.speex.org/))
* DBOPL AdLib emulator (taken from [DosBox](http://www.dosbox.com/))
* [Catch](https://github.com/philsquared/Catch) testing framework

### <a name="mac-build-instructions">OS X builds</a>

Building on OS X works almost exactly like the Linux build, except for getting
the dependencies. If you have Homebrew, you can get them using the following:

```bash
brew install cmake sdl2 sdl2_mixer boost
```

Note that you'll need Xcode 10 and OS X Mojave (10.14) if you want to use Apple's clang compiler. The project builds fine with a non-Apple clang though, so if you're on an older OS X version, you can still build it. Here's how you would install clang via Homebrew and build the project using it:

```bash
# You might need to run brew update
brew install llvm

# Set up environment variables so that CMake picks up the newly installed clang -
# this is only necessary the first time
export rigel_llvm_path=`brew --prefix llvm`;
export CC="$rigel_llvm_path/bin/clang";
export CXX="$CC++";
export CPPFLAGS="-I$rigel_llvm_path/include";
export LDFLAGS="-L$rigel_llvm_path/lib -Wl,-rpath,$rigel_llvm_path/lib";
unset rigel_llvm_path;

# Now, the regular build via CMake should work:
mkdir build
cd build
cmake .. -DWARNINGS_AS_ERRORS=OFF
make
```

### <a name="windows-build-instructions">Windows builds</a>

:exclamation: Currently, only 64-bit builds are possible.

First, you need to install CMake if you don't have it already.
You can grab it from [the Kitware website](https://cmake.org/download/), I went
for the `Windows win64-x64 Installer` variant.

For getting the dependencies, I strongly recommend using
[vcpkg](https://github.com/microsoft/vcpkg):


```bash
vcpkg install boost-program-options:x64-windows boost-algorithm:x64-windows sdl2:x64-windows sdl2-mixer:x64-windows --triplet x64-windows
```

Then pass `CMAKE_TOOLCHAIN_FILE=C:/path/to/your/vcpkgdir/scripts/buildystems/vcpkg.cmake` when invoking CMake.

When using Visual Studio 2019, CMake defaults to creating a 64-bit build. But when using Visual Studio 2017, you have to specify
this by passing `-G "Visual Studio 15 2017 Win64"` to CMake. The example below assumes you're using Visual Studio 2019.

```bash
mkdir build
cd build

# Remember to replace <vcpkg_root> with the path to where you installed vcpkg!
cmake .. -DWARNINGS_AS_ERRORS=OFF -DCMAKE_TOOLCHAIN_FILE=<vckpkg_root>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_GENERATOR_PLATFORM=x64

# This will open the generated Visual Studio solution
start RigelEngine.sln
```

***

<a name="foot-note-linux">[1]</a>: I'm using Linux Mint 18, based on Ubuntu 16.04 Xenial Xerus
