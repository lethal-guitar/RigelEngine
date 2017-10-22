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
#include <data/map.hpp>
#include <engine/collision_checker.hpp>
#include <engine/physical_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/timing.hpp>
#include <game_logic/interaction/elevator.hpp>
#include <game_logic/player_movement_system.hpp>


using namespace rigel;
using namespace game_logic;
using namespace engine;
using namespace player;
using namespace engine::components;
using namespace game_logic::components;


namespace ex = entityx;


TEST_CASE("Rocket elevator") {
  ex::EntityX entityx;
  auto player = entityx.entities.create();
  player.assign<WorldPosition>(6, 100);
  player.assign<Active>();
  initializePlayerEntity(player, true);

  auto& playerPosition = *player.component<WorldPosition>();
  auto& playerState = *player.component<PlayerControlled>();

  auto elevator = entityx.entities.create();
  elevator.assign<WorldPosition>(2, 103);
  elevator.assign<Active>();
  interaction::configureElevator(elevator);

  data::map::Map map{300, 300, data::map::TileAttributes{{0x0, 0xF}}};

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
  PhysicsSystem physicsSystem{&collisionChecker, &entityx.events};


  SECTION("Elevator is setup correctly") {
    CHECK(elevator.has_component<interaction::components::Elevator>());
    CHECK(elevator.has_component<SolidBody>());
    CHECK(elevator.has_component<BoundingBox>());

    REQUIRE(elevator.has_component<MovingBody>());
    CHECK(elevator.component<MovingBody>()->mGravityAffected == true);
  }

  auto& elevatorPosition = *elevator.component<WorldPosition>();

  MockServiceProvider mockServiceProvicer;
  interaction::ElevatorSystem elevatorSystem(player, &mockServiceProvicer);
  const auto runOneFrame =
    [&elevatorSystem, &physicsSystem, &entityx, &player](
      const PlayerInputState& inputState
    ) {
      elevatorSystem.update(entityx.entities, inputState);
      physicsSystem.update(entityx.entities);
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


  const auto idleState = PlayerInputState{};
  PlayerInputState movingUpState;
  movingUpState.mMovingUp = true;
  PlayerInputState movingDownState;
  movingDownState.mMovingDown = true;


  SECTION("Player attachment") {
    SECTION("Player is not attached when not completely standing on elevator") {
      runOneFrame(idleState);
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 5;
      runOneFrame(idleState);
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 4;
      runOneFrame(idleState);
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 1;
      runOneFrame(idleState);
      CHECK(!elevatorSystem.isPlayerAttached());
    }

    SECTION("Player is attached when completely on top of elevator") {
      playerPosition.x = 3;
      runOneFrame(idleState);
      CHECK(elevatorSystem.isPlayerAttached());

      playerPosition.x = 2;
      runOneFrame(idleState);
      CHECK(elevatorSystem.isPlayerAttached());
    }
  }

  playerPosition.x = 3;
  runOneFrame(idleState);


  SECTION("Movement on elevator") {
    SECTION("No movement while player idle") {
      const auto expectedPos = playerPosition;

      runOneFrame(idleState);

      verifyPositions(expectedPos);
    }

    SECTION("Moving up") {
      auto expectedPos = playerPosition;

      runOneFrame(movingUpState);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);

      runOneFrame(movingUpState);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Moving down") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;

      auto expectedPos = playerPosition;

      runOneFrame(movingDownState);

      expectedPos.y += 2;
      verifyPositions(expectedPos);

      runOneFrame(movingDownState);

      expectedPos.y += 2;
      verifyPositions(expectedPos);
    }

    SECTION("Elevator stays in air when player stops moving") {
      auto expectedPos = playerPosition;

      runOneFrame(movingUpState);

      runOneFrame(idleState);

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Elevator is detached when player jumps") {
      runOneFrame(movingUpState);

      const auto originalPos = playerPosition;
      const auto originalElevatorPos = elevatorPosition;

      player.component<MovingBody>()->mVelocity.y = -3.6f;
      player.component<MovingBody>()->mGravityAffected = true;
      player.component<PlayerControlled>()->mPerformedJump = true;
      runOneFrame(idleState);

      CHECK(!elevatorSystem.isPlayerAttached());
      CHECK(playerPosition.y < originalPos.y);
      CHECK(elevator.component<MovingBody>()->mVelocity.y == 2.0f);
      CHECK(elevatorPosition.y > originalElevatorPos.y);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Elevator is detached when player walks off") {
      // Setup: Get player+elevator in the air
      runOneFrame(movingUpState);
      runOneFrame(movingUpState);

      const auto originalPlayerY = playerPosition.y;
      const auto originalElevatorY = elevatorPosition.y;

      runOneFrame(idleState);

      playerPosition.x -= 2;
      runOneFrame(idleState);

      CHECK(!elevatorSystem.isPlayerAttached());
      CHECK(player.component<MovingBody>()->mGravityAffected);
      CHECK(playerPosition.y >= originalPlayerY);
      CHECK(elevatorPosition.y > originalElevatorY);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Moving down to ground doesn't get player stuck in elevator") {
      const auto expectedPos = playerPosition;

      // Setup: Get player+elevator in the air
      runOneFrame(movingUpState);

      CHECK(playerPosition.y == 98);
      CHECK(elevatorPosition.y == 101);

      runOneFrame(movingDownState);
      runOneFrame(movingDownState);

      verifyPositions(expectedPos);
    }

    SECTION("Player touching ceiling stops the elevator in position") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;
      const auto initialPos = playerPosition;

      runOneFrame(movingUpState);

      const auto expectedPos = initialPos - WorldPosition{0, 1};
      verifyPositions(expectedPos);

      runOneFrame(idleState);
      runOneFrame(idleState);

      verifyPositions(expectedPos);
    }
  }


  SECTION("Player state is updated correctly") {
    SECTION("State initially marked as not interacting") {
      runOneFrame(idleState);
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("State set to interacting when moving up") {
      runOneFrame(movingUpState);
      CHECK(playerState.mIsInteracting);
    }

    SECTION("State set to interacting when moving down") {
      runOneFrame(movingDownState);
      CHECK(playerState.mIsInteracting);
    }

    runOneFrame(movingUpState);

    SECTION("State reset to normal after movement stops") {
      runOneFrame(idleState);
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("State reset to normal after detaching") {
      playerPosition.x = 0;
      runOneFrame(idleState);
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("Interacting state is not changed when not attached to elevator") {
      playerPosition.x = 6;
      runOneFrame(idleState);

      playerState.mIsInteracting = true;
      runOneFrame(idleState);
      CHECK(playerState.mIsInteracting);
    }
  }
}
