# Rigel Engine [![Build status](https://ci.appveyor.com/api/projects/status/7yen9qaccci2vklw/branch/master?svg=true)](https://ci.appveyor.com/project/lethal-guitar/rigelengine/branch/master)

## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993. The end goal is to have a (mostly) functionally identical
runner/interpreter for the original game's files, but running natively on
modern operating systems, and written in modern C++ code. It is similar to
projects like [Omnispeak](https://davidgow.net/keen/omnispeak.html) or
[Commander Genius](http://clonekeenplus.sourceforge.net), which do the same
thing for the Commander Keen series of games.

There was never any source code released for the original game, so this project
is based on reverse-engineering: A mix of analyzing the original executable's
disassembly and observing its behaviour based on video captures of the game
running in DosBox.

## Current state

The project is still in an extremely early state. There are many areas of the
code which are rather hacky, early protoypes, and will be replaced in the future.
There is also not much functionality yet.

You can already load levels and walk/jump around in them, as well as collect
items.  Climbing ladders also works, but hanging onto climbing poles isn't
implemented yet.  There is not much interactivity otherwise, none of the
enemies do anything, you cannot take any damage, rocket elevators don't work
yet, you cannot shoot any weapon, and you can't finish levels.

Many of the game's other systems, like most of the menus, intro movie and story
sequences etc. are already implemented. Not all of them are currently
integrated though, e.g. the bonus screen which is shown in-between levels.

## Building and running

### Linux build quick start guide

If you're on Linux and running a recent enough Ubuntu/Debian-like distro, here's
how to quickly get the project up and running:

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

### Detailed build pre-requisites and dependencies

To build from source, a C++ 14 compatible compiler is required. The project has been
built successfully on the following compilers:

* Microsoft Visual Studio 2015 (Update 2 or newer) - see notes below
* gcc 5.4.0
* clang 3.9

The project depends on the following libraries:

* SDL >= 2.0
* SDL\_mixer >= 2.0
* Boost >= 1.58 (older versions might work, but I didn't try)

The following further dependencies are already provided as submodules or source
code (in the `3rd_party` directory):

* [entityx](https://github.com/alecthomas/entityx) Entity-Component-System framework, v. 1.2.0
* Speex Resampler (taken from [libspeex](http://www.speex.org/))
* DBOPL AdLib emulator (taken from [DosBox](http://www.dosbox.com/))
* [Catch](https://github.com/philsquared/Catch) testing framework
* [atria](https://github.com/Ableton/atria) C++ utilities

### Windows builds

Although the project can be built successfully on Windows, this is not yet fully
supported. In particular, there is no CMake find script for SDL 2 yet.
Full Windows support is planned for the future.
