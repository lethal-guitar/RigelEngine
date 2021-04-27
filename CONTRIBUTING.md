# Contribution guide

## Authenticity

RigelEngine aims to provide a very authentic experience by default, meaning gameplay should be largely identical to the original Duke Nukem II. Modern enhancements that alter the gameplay are welcome, but should be disabled by default, requiring the user to opt in via the options menu. If you are planning to contribute a feature that might alter gameplay, please add a corresponding option to toggle it on/off.

## Dependencies

The project requires very few external dependencies at the moment. While third-party libraries can provide a lot of value, having many dependencies also comes at a cost in the long run. Therefore, please carefully consider whether adding a new external dependency is absolutely necessary. If possible, consider making the dependency optional with a build-time switch, so that RigelEngine can still be used without the dependency if desired.

Having said that, I don’t see any reason against small dependencies which can be vendored alongside the source code (in the `3rd_party` directory), as long as they are supported on all current and potential future platforms (current: Windows, Mac OS X (x86, ARM), Linux (x86, ARM), Webassembly. Future: Android, iOS).

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
