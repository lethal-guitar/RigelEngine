## What is Rigel Engine?

This project is a re-implementation of the game [Duke Nukem II](
https://en.wikipedia.org/wiki/Duke_Nukem_II), originally released by Apogee
Software in 1993 for MS-DOS. RigelEngine works as a drop-in replacement for the original executable: It reads the game's data files and plays just like the original,
but runs natively on modern operating systems and is written in modern C++ code with a completely new architecture under the hood.
On top of that, it offers many modern enhancements, including:

* wide-screen mode
* smooth scrolling & movement mode with increased frame rate
* quick saving
* improved game controller support
* extended modding features

![Screenshot of RigelEngine, playing the first level of the game](./assets/screenshot.png)

There was never any source code released for the original game, so this project
is based on reverse engineering. Disassembly from the original executable served as basis for writing new code from scratch, while video captures from DosBox were used for frame-by-frame verification.
See [my blog post](https://lethalguitar.wordpress.com/2019/05/28/re-implementing-an-old-dos-game-in-c-17/) to learn more about the process.

Try the [web version](https://rigelengine.nikolai-wuttke.de)! (compiled to wasm via Emscripten)

Read the [F.A.Q.](https://github.com/lethal-guitar/RigelEngine/wiki/FAQ)

