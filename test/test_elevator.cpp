/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <catch.hpp>
#include "utils.hpp"

#include <base/spatial_types_printing.hpp>
#include <data/game_traits.hpp>
#include <data/map.hpp>
#include <data/player_model.hpp>
#include <engine/collision_checker.hpp>
#include <engine/entity_activation_system.hpp>
#include <engine/particle_system.hpp>
#include <engine/physical_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/random_number_generator.hpp>
#include <engine/timing.hpp>
#include <game_logic/behavior_controller_system.hpp>
#include <game_logic/entity_factory.hpp>
#include <game_logic/input.hpp>
#include <game_logic/interactive/elevator.hpp>
#include <game_logic/player.hpp>
#include <game_logic/player/components.hpp>


using namespace rigel;
using namespace game_logic;
using namespace engine;
using namespace engine::components;
using namespace game_logic::components;


namespace ex = entityx;


struct MockEventListener : public ex::Receiver<MockEventListener> {
  bool mIsPlayerAttached = false;

  void receive(const game_logic::events::ElevatorAttachmentChanged& e) {
    mIsPlayerAttached =
      e.mType == game_logic::events::ElevatorAttachmentChanged::Attach;
  }
};


struct MockSpriteFactory : public rigel::engine::ISpriteFactory {
  engine::components::Sprite createSprite(data::ActorID id) override {
    static rigel::engine::SpriteDrawData dummyDrawData;
    return {&dummyDrawData, {}};
  }

  base::Rect<int> actorFrameRect(data::ActorID id, int frame) const override {
    // Bounds for elevator
    return {{}, {4, 3}};
  }
};




TEST_CASE("Rocket elevator") {
  ex::EntityX entityx;

  data::map::Map map{300, 300, data::map::TileAttributeDict{{0x0, 0xF}}};

  // Floor
  for (int i = 0; i < 8; ++i) {
    const auto y = i > 1 && i < 6 ? 104 : 101;
    map.setTileAt(0, i, y, 1);
  }

  // Ceiling
  for (int i = 0; i < 10; ++i) {
    map.setTileAt(0, i, 90, 1);
  }

  CollisionChecker collisionChecker{&map, entityx.entities, entityx.events};
  data::PlayerModel playerModel;
  MockServiceProvider mockServiceProvider;
  engine::RandomNumberGenerator randomGenerator;
  MockSpriteFactory mockSpriteFactory;
  EntityFactory entityFactory{
    &mockSpriteFactory,
    &entityx.entities,
    &randomGenerator,
    data::Difficulty::Medium};

  auto playerEntity = entityx.entities.create();
  playerEntity.assign<WorldPosition>(6, 100);
  playerEntity.assign<Sprite>();
  assignPlayerComponents(playerEntity, Orientation::Left);

  Player player(
    playerEntity,
    data::Difficulty::Medium,
    &playerModel,
    &mockServiceProvider,
    &collisionChecker,
    &map,
    &entityFactory,
    &entityx.events,
    &randomGenerator);

  auto& playerPosition = player.position();

  auto elevator =
    entityFactory.spawnActor(data::ActorID::Rocket_elevator, {2, 103});

  base::Vector cameraPosition{0, 0};
  engine::ParticleSystem particleSystem{&randomGenerator, nullptr};
  PhysicsSystem physicsSystem{&collisionChecker, &map, &entityx.events};
  BehaviorControllerSystem behaviorControllerSystem{
    GlobalDependencies{
      &collisionChecker,
      &particleSystem,
      &randomGenerator,
      &entityFactory,
      &mockServiceProvider,
      &entityx.entities,
      &entityx.events},
    &player,
    &cameraPosition,
    &map};


  SECTION("Elevator is setup correctly") {
    CHECK(elevator.has_component<SolidBody>());
    CHECK(elevator.has_component<BoundingBox>());

    REQUIRE(elevator.has_component<MovingBody>());
    CHECK(elevator.component<MovingBody>()->mGravityAffected == true);
  }

  auto& elevatorPosition = *elevator.component<WorldPosition>();

  PerFrameState perFrameState;
  const auto runOneFrame = [&](const PlayerInput& input) {
    const auto& viewPortSize = data::GameTraits::mapViewPortSize;
    perFrameState.mInput = input;
    perFrameState.mCurrentViewPortSize = viewPortSize;

    player.update(input);
    engine::markActiveEntities(
      entityx.entities, {0, 0}, viewPortSize);
    behaviorControllerSystem.update(entityx.entities, perFrameState);
    physicsSystem.update(entityx.entities);
    perFrameState.mIsOddFrame = !perFrameState.mIsOddFrame;
  };

  const auto verifyPositions = [&playerPosition, &elevatorPosition](
    const WorldPosition& expectedPlayerPosition,
    const int elevatorHorizontalOffset = -1
  ) {
    const auto expectedElevatorPosition =
      expectedPlayerPosition + WorldPosition{elevatorHorizontalOffset, 3};
    CHECK(playerPosition == expectedPlayerPosition);
    CHECK(elevatorPosition == expectedElevatorPosition);
  };


  const auto noInput = PlayerInput{};
  PlayerInput pressingUp;
  pressingUp.mUp = true;

  PlayerInput pressingDown;
  pressingDown.mDown = true;

  PlayerInput jumpPressed;
  jumpPressed.mJump.mWasTriggered = true;
  jumpPressed.mJump.mIsPressed = true;

  MockEventListener listener;
  entityx.events.subscribe<game_logic::events::ElevatorAttachmentChanged>(
    listener);

  auto isPlayerAttached = [&]() {
    return listener.mIsPlayerAttached;
  };


  SECTION("Player attachment") {
    SECTION("Player is not attached when not completely standing on elevator") {
      runOneFrame(noInput);
      CHECK(!isPlayerAttached());

      playerPosition.x = 5;
      runOneFrame(noInput);
      CHECK(!isPlayerAttached());

      playerPosition.x = 4;
      runOneFrame(noInput);
      CHECK(!isPlayerAttached());

      playerPosition.x = 1;
      runOneFrame(noInput);
      CHECK(!isPlayerAttached());
    }

    SECTION("Player is attached when completely on top of elevator") {
      playerPosition.x = 3;
      runOneFrame(noInput);
      CHECK(isPlayerAttached());

      playerPosition.x = 2;
      runOneFrame(noInput);
      CHECK(isPlayerAttached());
    }
  }

  playerPosition.x = 3;
  runOneFrame(noInput);


  SECTION("Movement on elevator") {
    SECTION("No movement while player idle") {
      const auto expectedPos = playerPosition;

      runOneFrame(noInput);

      verifyPositions(expectedPos);
    }

    SECTION("Moving up") {
      auto expectedPos = playerPosition;

      runOneFrame(pressingUp);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);

      runOneFrame(pressingUp);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Moving down") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;

      auto expectedPos = playerPosition;

      runOneFrame(pressingDown);

      expectedPos.y += 2;
      verifyPositions(expectedPos);

      runOneFrame(pressingDown);

      expectedPos.y += 2;
      verifyPositions(expectedPos);
    }

    SECTION("Elevator stays in air when player stops moving") {
      auto expectedPos = playerPosition;

      runOneFrame(pressingUp);

      runOneFrame(noInput);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Elevator is detached when player jumps") {
      runOneFrame(pressingUp);

      const auto originalPos = playerPosition;
      const auto originalElevatorPos = elevatorPosition;

      // Player jump has one frame delay due to the "coil up" animation
      runOneFrame(jumpPressed);
      runOneFrame(noInput);

      CHECK(!isPlayerAttached());
      CHECK(playerPosition.y < originalPos.y);
      CHECK(elevator.component<MovingBody>()->mVelocity.y == 2.0f);
      CHECK(elevatorPosition.y > originalElevatorPos.y);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Elevator is detached when player walks off") {
      // Setup: Get player+elevator in the air
      runOneFrame(pressingUp);
      runOneFrame(pressingUp);

      const auto originalPlayerY = playerPosition.y;
      const auto originalElevatorY = elevatorPosition.y;

      runOneFrame(noInput);

      playerPosition.x -= 2;
      runOneFrame(noInput);

      CHECK(!isPlayerAttached());
      CHECK(playerPosition.y >= originalPlayerY);
      CHECK(elevatorPosition.y > originalElevatorY);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Moving down to ground doesn't get player stuck in elevator") {
      const auto expectedPos = playerPosition;

      // Setup: Get player+elevator in the air
      runOneFrame(pressingUp);

      CHECK(playerPosition.y == 98);
      CHECK(elevatorPosition.y == 101);

      runOneFrame(pressingDown);
      runOneFrame(pressingDown);

      verifyPositions(expectedPos);
    }

    SECTION("Player touching ceiling stops the elevator in position") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;
      const auto initialPos = playerPosition;

      runOneFrame(pressingUp);

      const auto expectedPos = initialPos - WorldPosition{0, 1};
      verifyPositions(expectedPos);

      runOneFrame(noInput);
      runOneFrame(noInput);

      verifyPositions(expectedPos);
    }
  }


  //SECTION("Player state is updated correctly") {
    //SECTION("State initially marked as not interacting") {
      //runOneFrame(noInput);
      //CHECK(!playerState.mIsInteracting);
    //}

    //SECTION("State set to interacting when moving up") {
      //runOneFrame(pressingUp);
      //CHECK(playerState.mIsInteracting);
    //}

    //SECTION("State set to interacting when moving down") {
      //runOneFrame(pressingDown);
      //CHECK(playerState.mIsInteracting);
    //}

    //runOneFrame(pressingUp);

    //SECTION("State reset to normal after movement stops") {
      //runOneFrame(noInput);
      //CHECK(!playerState.mIsInteracting);
    //}

    //SECTION("State reset to normal after detaching") {
      //playerPosition.x = 0;
      //runOneFrame(noInput);
      //CHECK(!playerState.mIsInteracting);
    //}

    //SECTION("Interacting state is not changed when not attached to elevator") {
      //playerPosition.x = 6;
      //runOneFrame(noInput);

      //playerState.mIsInteracting = true;
      //runOneFrame(noInput);
      //CHECK(playerState.mIsInteracting);
    //}
  //}
}
