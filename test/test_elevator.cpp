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

#include <base/spatial_types_printing.hpp>
#include <data/map.hpp>
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
  initializePlayerEntity(player, true);

  auto& playerPosition = *player.component<WorldPosition>().get();
  auto& playerState = *player.component<PlayerControlled>().get();

  auto elevator = entityx.entities.create();
  elevator.assign<WorldPosition>(2, 103);
  interaction::configureElevator(elevator);

  data::map::TileAttributes tileAttributes{{0x0, 0xF}};
  data::map::Map map{300, 300};

  // Floor
  for (int i = 0; i < 8; ++i) {
    const auto y = i > 1 && i < 6 ? 104 : 101;
    map.setTileAt(0, i, y, 1);
  }

  // Ceiling
  for (int i = 0; i < 10; ++i) {
    map.setTileAt(0, i, 90, 1);
  }

  PhysicsSystem physicsSystem{&map, &tileAttributes};


  SECTION("Elevator is setup correctly") {
    CHECK(elevator.has_component<interaction::components::Elevator>());
    CHECK(elevator.has_component<SolidBody>());
    CHECK(elevator.has_component<BoundingBox>());

    REQUIRE(elevator.has_component<Physical>());
    CHECK(elevator.component<Physical>()->mGravityAffected == true);
  }

  auto& elevatorPosition = *elevator.component<WorldPosition>().get();

  interaction::ElevatorSystem elevatorSystem(player);
  const auto update = [&elevatorSystem, &physicsSystem, &entityx, &player](
    const ex::TimeDelta dt
  ) {
    elevatorSystem.update(entityx.entities, entityx.events, dt);
    physicsSystem.update(entityx.entities, entityx.events, dt);
  };

  const auto runOneFrame = [&update]() {
    update(gameFramesToTime(1));
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


  SECTION("Player attachment") {
    SECTION("Player is not attached when not completely standing on elevator") {
      runOneFrame();
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 5;
      runOneFrame();
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 4;
      runOneFrame();
      CHECK(!elevatorSystem.isPlayerAttached());

      playerPosition.x = 1;
      runOneFrame();
      CHECK(!elevatorSystem.isPlayerAttached());
    }

    SECTION("Player is attached when completely on top of elevator") {
      playerPosition.x = 3;
      runOneFrame();
      CHECK(elevatorSystem.isPlayerAttached());

      playerPosition.x = 2;
      runOneFrame();
      CHECK(elevatorSystem.isPlayerAttached());
    }
  }

  playerPosition.x = 3;
  runOneFrame();

  const auto idleState = PlayerInputState{};
  PlayerInputState movingUpState;
  movingUpState.mMovingUp = true;
  PlayerInputState movingDownState;
  movingDownState.mMovingDown = true;


  SECTION("Movement on elevator") {
    SECTION("No movement while player idle") {
      const auto expectedPos = playerPosition;

      elevatorSystem.setInputState(idleState);

      runOneFrame();

      verifyPositions(expectedPos);
    }

    SECTION("Moving up") {
      auto expectedPos = playerPosition;

      elevatorSystem.setInputState(movingUpState);
      runOneFrame();

      expectedPos.y -= 2;
      verifyPositions(expectedPos);

      runOneFrame();

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Moving down") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;

      auto expectedPos = playerPosition;

      elevatorSystem.setInputState(movingDownState);
      runOneFrame();

      expectedPos.y += 2;
      verifyPositions(expectedPos);

      runOneFrame();

      expectedPos.y += 2;
      verifyPositions(expectedPos);
    }

    SECTION("Movement happens only once every game frame") {
      const auto expectedPos = playerPosition;

      elevatorSystem.setInputState(movingUpState);
      update(gameFramesToTime(1) / 2.0);

      verifyPositions(expectedPos);
    }

    SECTION("Elevator stays in air when player stops moving") {
      auto expectedPos = playerPosition;

      elevatorSystem.setInputState(movingUpState);
      runOneFrame();

      elevatorSystem.setInputState(idleState);
      runOneFrame();

      expectedPos.y -= 2;
      verifyPositions(expectedPos);
    }

    SECTION("Elevator is detached when player jumps") {
      elevatorSystem.setInputState(movingUpState);
      runOneFrame();
      elevatorSystem.setInputState(idleState);

      const auto originalPos = playerPosition;
      const auto originalElevatorPos = elevatorPosition;

      player.component<Physical>()->mVelocity.y = -3.6f;
      player.component<Physical>()->mGravityAffected = true;
      player.component<PlayerControlled>()->mPerformedJump = true;
      runOneFrame();

      CHECK(!elevatorSystem.isPlayerAttached());
      CHECK(playerPosition.y < originalPos.y);
      CHECK(elevator.component<Physical>()->mVelocity.y == 2.0f);
      CHECK(elevatorPosition.y > originalElevatorPos.y);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Elevator is detached when player walks off") {
      // Setup: Get player+elevator in the air
      elevatorSystem.setInputState(movingUpState);
      runOneFrame();
      runOneFrame();

      const auto originalPlayerY = playerPosition.y;
      const auto originalElevatorY = elevatorPosition.y;

      elevatorSystem.setInputState(idleState);
      runOneFrame();

      playerPosition.x -= 2;
      runOneFrame();

      CHECK(!elevatorSystem.isPlayerAttached());
      CHECK(player.component<Physical>()->mGravityAffected);
      CHECK(playerPosition.y >= originalPlayerY);
      CHECK(elevatorPosition.y > originalElevatorY);
      CHECK(elevator.has_component<SolidBody>());
    }

    SECTION("Moving down to ground doesn't get player stuck in elevator") {
      const auto expectedPos = playerPosition;

      // Setup: Get player+elevator in the air
      elevatorSystem.setInputState(movingUpState);
      runOneFrame();

      CHECK(playerPosition.y == 98);
      CHECK(elevatorPosition.y == 101);

      elevatorSystem.setInputState(movingDownState);
      runOneFrame();
      runOneFrame();

      verifyPositions(expectedPos);
    }

    SECTION("Player touching ceiling stops the elevator in position") {
      playerPosition.y = 96;
      elevatorPosition.y = 99;
      const auto initialPos = playerPosition;

      elevatorSystem.setInputState(movingUpState);
      runOneFrame();

      const auto expectedPos = initialPos - WorldPosition{0, 1};
      verifyPositions(expectedPos);

      elevatorSystem.setInputState(idleState);
      runOneFrame();
      runOneFrame();

      verifyPositions(expectedPos);
    }
  }


  SECTION("Player state is updated correctly") {
    SECTION("State initially marked as not interacting") {
      runOneFrame();
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("State set to interacting when moving up") {
      elevatorSystem.setInputState(movingUpState);
      runOneFrame();
      CHECK(playerState.mIsInteracting);
    }

    SECTION("State set to interacting when moving down") {
      elevatorSystem.setInputState(movingDownState);
      runOneFrame();
      CHECK(playerState.mIsInteracting);
    }

    elevatorSystem.setInputState(movingUpState);
    runOneFrame();

    SECTION("State reset to normal after movement stops") {
      elevatorSystem.setInputState(idleState);
      runOneFrame();
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("State reset to normal after detaching") {
      playerPosition.x = 0;
      runOneFrame();
      CHECK(!playerState.mIsInteracting);
    }

    SECTION("Interacting state is not changed when not attached to elevator") {
      playerPosition.x = 6;
      runOneFrame();

      playerState.mIsInteracting = true;
      runOneFrame();
      CHECK(playerState.mIsInteracting);
    }
  }
}
