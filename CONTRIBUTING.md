# Contribution guide

## Minimum requirements for code contributions

When contributing changes to RigelEngine's code,
please compile the code locally and try to run the game at least once
before submitting your PR.
There is a CI system in place which will test your changes on all platforms,
and I'm happy to help with fixing issues that are specific to other platforms than
the one you're working on.

Please do not submit PRs based on code changes made in GitHub's web interface,
as there's no way to know if the code actually compiles after these changes.

## Coding style and code quality guidelines

* Choose human readable names
* Prefer value semantics/value types if possible
* Avoid using raw `new` and `delete` (manual memory management). Use value types, standard containers, or standard smart pointers if necessary.
* Avoid using unsigned integer types except for bit masks and cases where defined overflow is necessary. For parsing binary data that contains unsigned integers, this is of course also ok. But most of the code should use `int` as the default numerical type.
* When a class/struct owns an object, hold it by value if possible. If the class/struct doesn't own the object, use a pointer. Please do not use reference members in classes/structs, as they prevent assignment.
* Dependencies should be passed via pointer on construction. Objects that are passed to a function to be operated on, but not taken ownership of, should be passed by reference.
* Avoid multi-step initialization: After construction, an object should be ready to use, without the need to call additional setup functions.
* Avoid global variables (static globals that are limited to one translation unit are okay)
* For polymorphism, prefer using `std::variant` & `base::match` instead of class hierarchies. If that's not an option, consider using [concept based polymorphism](https://www.youtube.com/watch?v=2bLkxj6EVoM). If that's not an option either, aim to make the class hierarchy as flat as possible. In the ideal case, there is one interface class (pure virtual only) and a couple of implementations, but no inheritance beyond that one level.
* Prefer free functions over member functions
* Prefer structs with named members over `std::tuple`/`std::pair`.

## Code formatting

RigelEngine uses [clang-format](https://releases.llvm.org/11.0.0/tools/clang/docs/ClangFormat.html) for automated code formatting. This is enforced by a check on CI, meaning your code needs to be clang-formatted before it can be merged. It's important to use exactly version 11 of clang-format, as unfortunately the formatting behavior varies between versions even with an identical configuration file.
Differences in minor or patch version seem to be ok, though, so 11.0.0 and 11.10.0 should both work fine.

To format the code in your working copy, you can either use the scripts `tools/format-all.sh` and `tools/format-staged.sh` directly (if clang-format 11 is already installed in your system), or run them via Docker (that way, you don't need to install it locally).

### Installing clang-format locally

On Ubuntu 20.04 or later, you can install clang-format 11 via `sudo apt-get install clang-format-11`.

For Windows, Mac, and other systems, you can [grab a binary clang/LLVM release](https://github.com/llvm/llvm-project/releases/tag/llvmorg-11.0.0).
Make sure `clang-format` is on your `PATH` after installing it.

Once installed, you can run `tools/format-staged.sh` or `./tools/format-all.sh` from the root of your working copy (using git bash on Windows).

### Running clang-format via Docker

First, build the Docker container as described [here](https://github.com/lethal-guitar/RigelEngine/blob/master/BUILDING.md#docker-build-instructions).

Once the container is built, you can run the following in the root of your working copy:

```bash
# Format files currently staged in git
docker run --rm -it -v $(pwd):/workdir -w /workdir rigel-build ./tools/format-staged.sh

# Format all files
docker run --rm -it -v $(pwd):/workdir -w /workdir rigel-build ./tools/format-all.sh
```

## Authenticity

RigelEngine aims to provide a very authentic experience by default, meaning gameplay should be largely identical to the original Duke Nukem II. Modern enhancements that alter the gameplay are welcome, but should be disabled by default, requiring the user to opt in via the options menu. If you are planning to contribute a feature that might alter gameplay, please add a corresponding option to toggle it on/off.

## Dependencies

The project requires very few external dependencies at the moment. While third-party libraries can provide a lot of value, having many dependencies also comes at a cost in the long run. Therefore, please carefully consider whether adding a new external dependency is absolutely necessary. If possible, consider making the dependency optional with a build-time switch, so that RigelEngine can still be used without the dependency if desired.

Having said that, I don’t see any reason against small dependencies which can be vendored alongside the source code (in the `3rd_party` directory), as long as they are supported on all current and potential future platforms (current: Windows, Mac OS X (x86, ARM), Linux (x86, ARM), Webassembly. Future: Android, iOS).
