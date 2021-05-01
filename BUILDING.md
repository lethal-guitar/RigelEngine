# How to build RigelEngine from source

* [Cloning & initializing submodules](#get-the-sources)
* [A note about warnings as errors](#a-note-about-warnings-as-errors)
* [Pre-requisites and dependencies](#dependencies)
* [Linux builds](#linux-build-instructions)
    * [Ubuntu 19.04 or newer](#linux-build-instructions-194)
    * [Ubuntu 18.04](#linux-build-instructions-184)
    * [Fedora 31 or newer](#linux-build-instructions-fedora)
* [Raspberry Pi builds](#raspi-build-instructions)
* [Docker builds](#docker-build-instructions)
* [OS X builds](#mac-build-instructions)
    * [Catalina (10.15) or newer](#mac-build-instructions-1015)
    * [Mojave (10.14) using clang 8](#mac-build-instructions-1014)
    * [High Sierra (10.13) or older using clang 7](#mac-build-instructions-1013)
* [Windows builds](#windows-build-instructions)
    * [64-bit builds](#windows-build-instructions-64)
    * [32-bit builds](#windows-build-instructions-32)
* [Webassembly (Emscripten) builds](#wasm-build-instructions)

### <a name="get-the-sources">Cloning & initializing submodules</a>

RigelEngine uses Git submodules, which means you need to either use `git clone --recursive`,
or run one additional command after cloning to initialize submodules:

```bash
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

The project depends on the following libraries, which CMake needs to be able
to find for the build to work:

* SDL >= 2.0.4
* SDL\_mixer >= 2.0.1
* Boost >= 1.65

All other dependencies are already provided as submodules or source
code (in the `3rd_party` directory), and will be built along with RigelEngine.
In other words, you don't need to worry about providing these dependencies.

These are:

* [Catch](https://github.com/philsquared/Catch) testing framework
* DBOPL AdLib emulator (taken from [DosBox](http://www.dosbox.com/))
* [Dear ImGui](https://github.com/ocornut/imgui) UI framework
* [entityx](https://github.com/alecthomas/entityx) Entity-Component-System framework
* [File browser extension](https://github.com/AirGuanZ/imgui-filebrowser) for Dear ImGui
* [Glad](https://github.com/Dav1dde/glad) OpenGL function loader generator
* [GLM](https://github.com/g-truc/glm.git) math library
* [JSON for modern C++](https://github.com/nlohmann/json)
* Speex Resampler (taken from [libspeex](http://www.speex.org/))
* [STB image](https://github.com/nothings/stb) image reading/writing library
* [STB rect_pack](https://github.com/nothings/stb) rectangle packer for building texture atlases


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

:exclamation: Make sure to read [Running on Raspberry Pi](https://github.com/lethal-guitar/RigelEngine/wiki/Running-on-Raspberry-Pi-and-Odroid-Go-Advance#raspberry-pi) for information on how to achieve best performance!

To build on the Pi itself, I recommend Raspbian (aka Raspberry Pi OS) _Buster_.
Older versions like Stretch don't have recent enough versions of CMake, Boost and Gcc.

Installing the dependencies on Buster works exactly like [on Ubuntu](#linux-build-instructions-194).

When building, you need to enable OpenGL ES Support:

```bash
mkdir build
cd build
cmake .. -DUSE_GL_ES=ON -DCMAKE_BUILD_TYPE=Release -DWARNINGS_AS_ERRORS=OFF
make
```

### <a name="docker-build-instructions">Docker builds</a>

The provided docker image allows you to mimic one of the CI environments (ubuntu:latest).

Build the image using

```bash
docker build . -f docker/ubuntu.Dockerfile -t rigel-build
```

Then run your container with:

```bash
# build it, debug it, etc..
docker run -ti --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -v $(pwd):/workdir -w /workdir rigel-build
```

And now follow the build steps from [Ubuntu](#linux-build-instructions-194).

#### Running it inside the container

You can also run the game from the container by following these instructions:

1) on your host: `xhost +` (will allow connections from anything - you can be more granular if you wish)
2) run the same build container with:
```bash
# ability to run the game with the GPU mapped + X forwarding
docker run -ti --device=/dev/dri:/dev/dri --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --net=host -v $(pwd):/workdir -w /workdir rigel-build
```
3) build & run, same as you would on your local machine

### <a name="mac-build-instructions">OS X builds</a>

:exclamation: On Mojave (10.14) and older, non-Apple clang must be installed via Homebrew, Apple's clang does not have all required C++ library features. Starting with Catalina (10.15), Xcode's clang works fine.

The oldest OS/compiler combination that has worked for me is clang 7 on Sierra (10.12).

### <a name="mac-build-instructions-1015">Catalina (10.15) or newer</a>

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

### <a name="mac-build-instructions-1014">Mojave (10.14) using clang 8</a>

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

### <a name="mac-build-instructions-1013">High Sierra (10.13) or older using clang 7</a>

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

#### <a name="windows-build-instructions-64">64-bit builds</a>

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

#### <a name="windows-build-instructions-32">32-bit builds</a>

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

