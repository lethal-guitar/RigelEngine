# Game logic code

This folder hosts the bulk of RigelEngine's game logic. The vast majority of this
code is based on reverse engineering the original game's executable. It
uses the [entityx](https://github.com/alecthomas/entityx) Entity-Component-System library, so a lot of the code in here is components and systems.
Most actors/game objects should nowadays be implemented by using the `BehaviorController`
mechanism though (see [guide in the wiki](https://github.com/lethal-guitar/RigelEngine/wiki/Implementing-new-enemies-or-actors)).

The main facade for the game logic is the `GameWorld` class, which ties everything
together and provides a simple interface to the outside. This class doesn't know
anything about timing/speed, it's the responsibility of client code to run the
game at whatever speed it wants to. The `GameRunner` class (outside this folder)
takes care of running the game at the right speed based on actual user input.
But `GameWorld` could also be used to play the game based on synthetic input etc.

Another important class is `EntityFactory`, which knows how to create entities
for given actor IDs.

The most relevant files for adding new type of game object/actor:

* `destruction_effect_specs.ipp`
* `entity_configuration.ipp`
* `entity_factory.cpp`

## Sub-folders

### `enemies`

Contains logic to implement enemy behavior. Code for new enemies should go in here.

### `hazards`

Similar to enemies, but for "hazards", i.e. game objects that can damage the player,
but unlike enemies, they don't move around and can't be destroyed.

### `interactive`

Game objects that the player can interact with in some way.

### `player`

Player-specific logic. Note that the bulk of the player control code lives in the
`Player` class at the top level of the `game_logic` directory.
