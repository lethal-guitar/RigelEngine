# Structure of the code base

RigelEngine's source code is organized into a couple of packages/modules, each of which has its own
source folder. All these modules are compiled into a static library
`rigel_core`. Some additional files are not assigned to any module,
but reside directly in the top level of the `src` directory. These
files are compiled into the actual RigelEngine executable, which
links against the `rigel_core` library. Unit tests (in the `tests` directory) link against
the same library as well.

## Modules

### `audio`

Contains code related to sound & music playback.

### `assets`

Code for working with the file formats used by the original Duke Nukem II game. Main purpose is to
extract various assets like images, sounds, music, level data etc. and convert them into data structures
that can be used by the rest of the code, using data types from the `data` module. The idea is that all
the messy details of the original file formats are abstracted away by this module, so that the rest of the
code doesn't need to be aware of them. For example, the original game uses 16-color graphics with a palette.
The loader module converts these into modern 32-bit RGBA graphics, which makes it easy to
create OpenGL textures.

This module depends only on `base`, `data`, the standard library, and some 3rd party code.

### `base`

This module contains fundamental utilities and data structures. It can
be seen as an extension of the C++ standard library, providing additional functionality
required by the rest of the code base.

Should only depend on the standard library ideally (there is currently a dependency on SDL
in an implementation detail, but this should be removed longer term).

### `common`

Contains code that's relevant for both the core (i.e. `rigel_core`) and the executable (`RigelEngine`).

### `data`

Contains data structures for all kinds of game data/state, and various constants. Code in the
`loader` module typically produces data using types from this module.

Code in here only depends on `base` and the standard library, which should ideally be kept that way.

### `engine`

Lower level building blocks, on which `game_logic` is built upon. Uses entityx,
and defines some components and systems.

Mostly quite specific to Duke Nukem, but a bit less so than `game_logic`. The lines between the two are a little
blurry, and could be refined in the future. A (incomplete) list of responsibilities:

* Collision detection
* Rendering
* Entity movement/physics
* Controlling entity life time

### `frontend`

Contains the top-level game mode management.
The game can be in three top-level modes: Intro/demo, main menu, and in-game.
Each of these is represented by a corresponding class.
The `Game` class implements mode management and ties together all the infrastructure required for the game modes to do their job.
For the platform startup code, it acts as the main facade for the game.

### `game_logic`

This is where all the game logic is implemented. See corresponding README for more
details.

### `renderer`

Provides 2d-rendering functionality, using OpenGL as backend. This is meant
to be kept fairly small in terms of the public interface, and ideally general enough
that other types of games could be built on top of it.

### `sdl_utils`

Some utilities to make it easier to work with SDL (a C library) in C++.

### `ui`

Contains some general UI-related building blocks, UI components like the HUD, and implementations of some of Duke Nukem's screens/modes. More specifically:

* Intro movie and Apogee Logo movie (the movies are stored in files, but they have no sound effects, and are missing timing information. These aspects are controlled by code in the original executable. The classes here represent RigelEngine's implementation of that logic).
* Bonus Screen (shown between levels)
* Episode End screen (short story sequence after beating a boss)
* High Score list
* Options Menu
* In-game Menu

Most other menus, screens etc. in Duke Nukem are implemented using [DukeScript](https://github.com/lethal-guitar/RigelEngine/wiki/DukeScript). `DukeScriptRunner`, which implements DukeScript for RigelEngine, can also be found here.

## "Main" code

The remaining code consists of:
* `main.cpp`: command line option parsing
* `game_main`: startup and initialization, main loop
* `emscripten_main.cpp`: startup and main loop setup for Webassembly builds using Emscripten
