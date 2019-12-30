# Rigel Engine [![Build Status](https://api.travis-ci.com/lethal-guitar/RigelEngine.svg?branch=master)](https://travis-ci.com/lethal-guitar/RigelEngine) [![Windows build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master) [![Join the chat at https://gitter.im/RigelEngine/community](https://badges.gitter.im/RigelEngine/community.svg)](https://gitter.im/RigelEngine/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![GitHub All Releases](https://img.shields.io/github/downloads/lethal-guitar/RigelEngine/total.svg?style=flat)](https://github.com/lethal-guitar/RigelEngine/releases)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993. RigelEngine works as a drop-in replacement for the original executable: It reads the original data files and offers the same experience (plus some improvements),
but runs natively on modern operating systems, and is written in modern C++ code with a completely new architecture under the hood. It is similar to
projects like [Omnispeak](https://davidgow.net/keen/omnispeak.html) or [Commander Genius](http://clonekeenplus.sourceforge.net), which do the same
thing for the Commander Keen series of games.

There was never any source code released for the original game, so this project
is based on reverse-engineering the original executable's assembly code. Video captures from DosBox are used to verify re-implemented behavior.
See [my blog post](https://lethalguitar.wordpress.com/2019/05/28/re-implementing-an-old-dos-game-in-c-17/) to learn more about the process.

Here's a video showcasing the project:

<a href="http://www.youtube.com/watch?v=Z3gCS5LvC2s"><img src="https://i.imgur.com/06btu7R.png" width="540"></img></a>

## Current state

RigelEngine fully supports the shareware version of the original game.
Support for the registered version is almost finished, but the AI for the boss enemy in episode 2 isn't implemented yet.
In addition, the levels from the registered version haven't been thoroughly tested yet, so there might be bugs.

The only other major feature that's still missing is demo playback.

Plus, it would be nice to make launching the game easier in the future by having a UI for selecting the path to the game files, and possibly even downloading the Shareware files automatically.

## Contributing

Contributions to RigelEngine are very welcome! Please have a look at the [contribution guide](CONTRIBUTING.md) before making a PR.

There is a growing body of documentation on the Wiki, to help with getting into the code base. A good place to start is [Architecture Overview](https://github.com/lethal-guitar/RigelEngine/wiki/Architecture-overview)

If you are looking for some easy tasks to get started, take a look at issues labeled [good first issue](https://github.com/lethal-guitar/RigelEngine/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22).

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
* `-h`/`--help`: show all command line options

### Debugging tools, more info

You can find more info that didn't quite fit in this README over on [the Wiki](https://github.com/lethal-guitar/RigelEngine/wiki). For example, you'll find info on how to activate the built-in debugging tools, a list of bugs in the original version that have been fixed in Rigel Engine, etc.

## Getting binaries

Pre-built binaries are provided for Windows. You can grab them from the [Releases tab](https://github.com/lethal-guitar/RigelEngine/releases). Alternatively, you can grab a build of the latest `master` branch by going to [AppVeyor](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master), clicking on "Configuration: Release", and then clicking on "Artifacts".

I'm planning to also provide binaries for OS X and Linux in the future. But right now, you have to build the project yourself on these platforms.

## Building from source

* [Get the sources](#get-the-sources)
* [A note about warnings as errors](#a-note-about-warnings-as-errors)
* [Pre-requisites and dependencies](#dependencies)
* [Linux builds](#linux-build-instructions)
    * [Ubuntu 19.04 or newer](#linux-build-instructions-194)
    * [Ubuntu 18.04](#linux-build-instructions-184)
* [OS X builds](#mac-build-instructions)
* [Windows builds](#windows-build-instructions)

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

### <a name="dependencies">Pre-requisites and dependencies</a>

To build from source, a C++ 17 compatible compiler is required. The project has been
built successfully on the following compilers:

* Microsoft Visual Studio 2019 (version 16.1.6 or newer)
* gcc 8.1.0
* clang 7.0.0

Slightly older versions of gcc/clang might also work, but I haven't tried that.

CMake version 3.12 or newer is required in order to generate build files.

The project depends on the following libraries:

* SDL >= 2.0.4
* SDL\_mixer >= 2.0.1
* Boost >= 1.65

The following further dependencies are already provided as submodules or source
code (in the `3rd_party` directory):

* [entityx](https://github.com/alecthomas/entityx) Entity-Component-System framework, v. 1.1.2
* Speex Resampler (taken from [libspeex](http://www.speex.org/))
* DBOPL AdLib emulator (taken from [DosBox](http://www.dosbox.com/))
* [Catch](https://github.com/philsquared/Catch) testing framework


### <a name="linux-build-instructions">Linux builds</a>

In order to be able to install all required dependencies from the system's
package manager, a fairly recent distro is required. I have successfully built
the project using the following instructions on Ubuntu 19.04 and 19.10.
Building on Ubuntu 18.04 is also possible, but it requires a few more steps.

#### <a name="linux-build-instructions-194">Ubuntu 19.04 or newer</a>

```bash
# Install all external dependencies, as well as the CMake build system:
sudo apt-get install cmake libboost-all-dev libsdl2-dev libsdl2-mixer-dev

# Configure and build (run inside your clone of the repo):
mkdir build
cd build
cmake .. -DWARNINGS_AS_ERRORS=OFF
make -j8

# NOTE: The -j<NUM_PROCESSES> argument to 'make' enables multi-core
# compilation, '8' is a good number for a 4-core machine with hyperthreading.
# You may want to tweak this number depending on your actual CPU configuration.
#
# If you plan to develop RigelEngine, I recommend dropping the
# -DWARNINGS_AS_ERRORS part - see the note about warnings as errors above.

# Now run it!
./src/RigelEngine <PATH_TO_YOUR_GAME_FILES>
```

#### <a name="linux-build-instructions-184">Ubuntu 18.04</a>

Ubuntu 18.04 almost provides everything we need out of the box, aside from
CMake and gcc. Fortunately, we can install gcc 8 alongside the system
default of gcc 7, and it's available in the package manager. CMake, however,
has to be built from source or installed via a PPA.

```bash
# Install all external dependencies, and gcc 8. CMake will be built from source.
sudo apt-get install g++-8 libboost-all-dev libsdl2-dev libsdl2-mixer-dev

# Now we need to install a newer version of CMake. If you already have CMake
# installed, you can uninstall it by running:
#
# sudo apt purge --auto-remove cmake
#
# If not, proceed directly with the following:
mkdir ~/temp
cd ~/temp
wget https://github.com/Kitware/CMake/releases/download/v3.15.4/cmake-3.15.4.tar.gz
tar -xzvf cmake-3.15.4.tar.gz
cd cmake-3.15.4

./bootstrap
make -j8 # adjust depending on number of CPU cores in your machine
sudo make install

# Now, when you run cmake --version, it should say 3.15. You can delete the
# ~/temp folder.

# Now we can build. Navigate to where you've cloned the repo, then:
mkdir build
cd build

# Now we need to tell CMake to use gcc 8 instead of the system default
# (which is 7). You only need to do this when running CMake for the first time.
export CC=`which gcc-8`
export CXX=`which g++-8`

# Finally, we can configure and build as usual (see above).
cmake .. -DWARNINGS_AS_ERRORS=OFF
make -j8 # adjust depending on number of CPU cores in your machine

# Now run it!
./src/RigelEngine <PATH_TO_YOUR_GAME_FILES>
```

### <a name="mac-build-instructions">OS X builds</a>

:exclamation: Currently, the project needs to be built using clang installed via Homebrew - it does _not_ build successfully using Apple's clang.

Note that you'll need Xcode 10 and OS X Mojave (10.14) if you want to use clang 8. In the past, I have successfully built the project on OS X Sierra (10.12) when using clang 7 (`brew install llvm@7`), and I'm not aware of any reason why the project shouldn't build on clang 7 anymore. However, I'm building with clang 8 these days, so it's possible that something broke.

Here's how you would install all dependencies as well as clang 8 via Homebrew and build the project using it:

```bash
# You might need to run brew update
brew install llvm@8 cmake sdl2 sdl2_mixer boost

# Set up environment variables so that CMake picks up the newly installed clang -
# this is only necessary the first time
export rigel_llvm_path=`brew --prefix llvm@8`;
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

```bash
mkdir build
cd build

# Remember to replace <vcpkg_root> with the path to where you installed vcpkg!
cmake .. -DWARNINGS_AS_ERRORS=OFF -DCMAKE_TOOLCHAIN_FILE=<vckpkg_root>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_GENERATOR_PLATFORM=x64

# This will open the generated Visual Studio solution
start RigelEngine.sln
```
