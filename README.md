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

It also runs on the Raspberry Pi, but that's work in progress as there are still [some issues](https://github.com/lethal-guitar/RigelEngine/labels/raspberry-pi-support) to be sorted out. See [build instructions](#raspi-build-instructions).

Android and iOS versions might happen someday, but there are no concrete plans at the moment.

### System requirements

RigelEngine is not very demanding, but it does require OpenGL-capable graphics hardware.
Either OpenGL 3.0 or OpenGL ES 2.0 can be used, depending on what's chosen at compile time.
To build in GL ES mode, pass `-DUSE_GL_ES=ON` to CMake.

Most Desktop/laptop graphics cards support OpenGL 3.0 nowadays.
However, some older integrated GPUs might only support OpenGL 2.
For these systems, using GL ES can be an option as well.
This has been confirmed to work on Ubuntu 18.04 on an older laptop.

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

Pre-built binaries for Windows are provided with each [Release](https://github.com/lethal-guitar/RigelEngine/releases). You can also grab a build of the latest `master` branch by going to [AppVeyor](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master), clicking on "Configuration: Release", and then clicking on "Artifacts".

Thanks to [@mnhauke](https://github.com/mnhauke), there is now also a [Linux package for OpenSUSE Tumbleweed](https://software.opensuse.org/package/RigelEngine).

I'm planning to provide binaries for OS X, Ubuntu/Debian, and Raspberry Pi in the future, but right now, you need to build the project yourself on these platforms.

## Building from source

* [Get the sources](#get-the-sources)
* [A note about warnings as errors](#a-note-about-warnings-as-errors)
* [Pre-requisites and dependencies](#dependencies)
* [Linux builds](#linux-build-instructions)
    * [Ubuntu 19.04 or newer](#linux-build-instructions-194)
    * [Ubuntu 18.04](#linux-build-instructions-184)
    * [Fedora 31 or newer](#linux-build-instructions-fedora)
* [Raspberry Pi builds](#raspi-build-instructions)
* [OS X builds](#mac-build-instructions)
* [Windows builds](#windows-build-instructions)
* [Webassembly (Emscripten) builds](#wasm-build-instructions)

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

CMake version 3.13 or newer is required in order to generate build files.

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

For other distros (not based on Debian), the same instructions should work as well,
with only the package installation command needing adaptation,
as long as all necessary dependencies are available at recent enough versions.

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
./src/RigelEngine
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
./src/RigelEngine
```

#### <a name="linux-build-instructions-fedora">Fedora 31 or newer</a>

On Fedora, the following command installs all
required dependencies:

```bash
sudo dnf install cmake boost-devel boost-program-options boost-static SDL2-devel SDL2_mixer-devel
```

Note the additional `boost-static` package - without it, there will be linker errors.

This also assumes that `make`, `gcc` and `gcc-c++` are already installed.

### <a name="raspi-build-instructions">Raspberry Pi builds</a>

:warning: Note that Raspberry Pi support is still work in progress, and there are [some issues](https://github.com/lethal-guitar/RigelEngine/labels/raspberry-pi-support).

To build on the Pi itself, I recommend Raspbian (aka Raspberry Pi OS) _Buster_.
Older versions like Stretch don't have recent enough versions of CMake, Boost and Gcc.

Installing the dependencies on Buster works exactly like [on Ubuntu](#linux-build-instructions-194).

When building, you need to enable OpenGL ES Support:

```bash
mkdir build
cd build
cmake .. -DUSE_GL_ES=ON -DWARNINGS_AS_ERRORS=OFF
make
```

To get playable performance, I had to run the game outside of the Desktop environment (X server).
To do that, switch to a new terminal using Ctrl+Alt+F1 and launch the game there.

### <a name="mac-build-instructions">OS X builds</a>

:exclamation: On Mojave (10.14) and older, non-Apple clang must be installed via Homebrew, Apple's clang does not have all required C++ library features. Starting with Catalina (10.15), Xcode's clang works fine.

The oldest OS/compiler combination that has worked for me is clang 7 on Sierra (10.12).

### Catalina (10.15) or newer

```
# Install dependencies
# You might need to run brew update.
brew install cmake sdl2 sdl2_mixer boost

# Configure & build
mkdir build
cd build
cmake .. -DWARNINGS_AS_ERRORS=OFF
make
```

### Mojave (10.14) using clang 8 

:exclamation: Note that you'll need Xcode 10.

```bash
# You might need to run brew update.
brew install llvm@8 cmake sdl2 sdl2_mixer boost

# Set up environment variables so that CMake picks up the newly installed clang -
# this is only necessary the first time.
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

### High Sierra (10.13) or older using clang 7

```bash
# You might need to run brew update.
brew install llvm@7 cmake sdl2 sdl2_mixer boost

# Set up environment variables so that CMake picks up the newly installed clang -
# this is only necessary the first time.
export rigel_llvm_path=`brew --prefix llvm@7`;
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

:exclamation: For best performance, make sure to switch to "Release" mode in Visual Studio before building.

First, you need to install CMake if you don't have it already.
You can grab it from [the Kitware website](https://cmake.org/download/), I went
for the `Windows win64-x64 Installer` variant.

For getting the dependencies, I strongly recommend using
[vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows).
You then need to pass `CMAKE_TOOLCHAIN_FILE=C:/path/to/your/vcpkgdir/scripts/buildystems/vcpkg.cmake` when invoking CMake.

:exclamation: Make sure to specify the toolchain file as _absolute path_. If the toolchain file path is incorrect, CMake will just silently ignore it, resulting in "package not found" errors.
So in case of errors, I'd recommend double checking the path first.

#### 64-bit builds

```bash
# Install dependencies
vcpkg install boost-program-options:x64-windows boost-algorithm:x64-windows sdl2:x64-windows sdl2-mixer:x64-windows --triplet x64-windows

# Run CMake
mkdir build
cd build

# Remember to replace <vcpkg_root> with the path to where you installed vcpkg!
cmake .. -DWARNINGS_AS_ERRORS=OFF -DCMAKE_TOOLCHAIN_FILE=<vckpkg_root>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_GENERATOR_PLATFORM=x64

# This will open the generated Visual Studio solution
start RigelEngine.sln
```

#### 32-bit builds

:exclamation: Currently, only 64-bit builds are regularly tested and built on CI. Building for 32-bit is possible, but there might be small build errors from time to time.
If you'd like to build for 32-bit and have trouble sorting out build errors, feel free to [open an issue](https://github.com/lethal-guitar/RigelEngine/issues/new/choose) and I'll look into it.

```bash
# Install dependencies
vcpkg install boost-program-options:x86-windows boost-algorithm:x86-windows sdl2:x86-windows sdl2-mixer:x86-windows --triplet x86-windows

# Run CMake
mkdir build
cd build

# Remember to replace <vcpkg_root> with the path to where you installed vcpkg!
cmake .. -DWARNINGS_AS_ERRORS=OFF -DCMAKE_TOOLCHAIN_FILE=<vckpkg_root>/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows -DCMAKE_GENERATOR_PLATFORM=Win32

# This will open the generated Visual Studio solution
start RigelEngine.sln
```

### <a name="wasm-build-instructions">Webassembly (Emscripten) build </a>

Rigel Engine can be built to run on a web browser using [Emscripten](https://emscripten.org/).
Note that this is still work in progress, as there are some [issues](https://github.com/lethal-guitar/RigelEngine/milestone/16) to resolve.

Refer to Emscripten's [README](https://github.com/emscripten-core/emsdk) for instructions on how to install and activate Emscripten.
You can verify the installation by running the `emcc` command. If it executes without errors then everything is configured correctly.

The instructions to build are the same as above, except for the CMake part:

```bash
emcmake cmake .. -DWARNINGS_AS_ERRORS=OFF -DWEBASSEMBLY_GAME_PATH=<path-to-duke2-folder>
make
```

This will produce a couple of files in the `<path-to-cmake-build-dir>/src` folder.
To run the game, you need to run a web server to host the files in that folder,
and then open `RigelEngine.html` in your browser.
Just opening `RigelEngine.html` directly will not work, as the web page needs to load the wasm code and data, which most browsers don't allow when using the local `file:///` protocol.

The easiest way to host the files is using Python. In the CMake build directory, run:

```bash
# Python 2
python -m SimpleHTTPServer

# Python 3
python3 -m http.server
```

This will host the game at `localhost:8000/src/RigelEngine.html`.
