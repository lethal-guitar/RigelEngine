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
#include <base/warnings.hpp>
#include <data/player_model.hpp>
#include <game_logic/player/attack_system.hpp>
#include <game_logic/player_movement_system.hpp>
#include <game_mode.hpp>

RIGEL_DISABLE_WARNINGS
#include <atria/testing/spies.hpp>
RIGEL_RESTORE_WARNINGS

#include <iostream>


using namespace std;

using namespace rigel;
using namespace game_logic;

using engine::components::WorldPosition;
using game_logic::components::PlayerControlled;
using game_logic::player::PlayerState;

namespace ex = entityx;


struct FireShotParameters {
  ProjectileType type;
  WorldPosition position;
  ProjectileDirection direction;
};


namespace rigel { namespace data {

static std::ostream& operator<<(std::ostream& os, const SoundId id) {
  os << static_cast<int>(id);
  return os;
}

}}


TEST_CASE("Player attack system works as expected") {
  ex::EntityX entityx;
  auto player = entityx.entities.create();
  player.assign<WorldPosition>(0, 0);
  initializePlayerEntity(player, true);

  data::PlayerModel playerModel;

  FireShotParameters fireShotParameters;
  auto fireShotSpy = atria::testing::spy([&fireShotParameters](
    const ProjectileType type,
    const WorldPosition& position,
    const ProjectileDirection direction
  ) {
    fireShotParameters.type = type;
    fireShotParameters.position = position;
    fireShotParameters.direction = direction;
  });

  struct MockEntityFactory {
    std::function<void(
      const ProjectileType type,
      const WorldPosition& position,
      const ProjectileDirection direction)> createProjectile;
  } mockEntityFactory{fireShotSpy};


  MockServiceProvider mockServiceProvicer;
  player::AttackSystem<MockEntityFactory> attackSystem{
    player,
    &playerModel,
    &mockServiceProvicer,
    &mockEntityFactory};

  const auto defaultInputState = PlayerInputState{};
  auto shootingInputState = PlayerInputState{};
  shootingInputState.mShooting = true;

  const auto update = [&attackSystem](const auto& inputState) {
    attackSystem.buttonStateChanged(inputState);
    attackSystem.update();
  };

  REQUIRE(player.has_component<PlayerControlled>());
  auto& playerState = *player.component<PlayerControlled>();
  auto& playerPosition = *player.component<WorldPosition>();

  SECTION("Pressing fire button triggers only one shot") {
    update(defaultInputState);
    CHECK(fireShotSpy.count() == 0);

    update(shootingInputState);
    CHECK(fireShotSpy.count() == 1);

    // holding the fire button doesn't trigger another shot
    update(shootingInputState);
    CHECK(fireShotSpy.count() == 1);

    SECTION("Releasing fire button allows shooting again on next press") {
      update(defaultInputState);

      update(shootingInputState);
      CHECK(fireShotSpy.count() == 2);
    }
  }


  SECTION("Shot position and direction depend on player orientation and state") {
    SECTION("Standing") {
      SECTION("Facing right") {
        update(shootingInputState);

        const auto expectedPosition = WorldPosition{3, -2};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Right);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        update(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -2};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Left);
      }

      SECTION("Player position offset") {
        playerPosition.x += 4;

        SECTION("Facing right") {
          update(shootingInputState);
          CHECK(fireShotParameters.position.x == 3 + 4);
        }

        SECTION("Facing left") {
          playerState.mOrientation = player::Orientation::Left;

          update(shootingInputState);
          CHECK(fireShotParameters.position.x == -1 + 4);
        }
      }
    }

    SECTION("Crouching") {
      playerState.mState = PlayerState::Crouching;

      SECTION("Facing right") {
        update(shootingInputState);

        const auto expectedPosition = WorldPosition{3, -1};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Right);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        update(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -1};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Left);
      }
    }

    SECTION("Looking up") {
      playerState.mState = PlayerState::LookingUp;

      SECTION("Facing right") {
        update(shootingInputState);

        const auto expectedPosition = WorldPosition{2, -5};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Up);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        update(shootingInputState);

        const auto expectedPosition = WorldPosition{0, -5};
        CHECK(fireShotParameters.position == expectedPosition);
        CHECK(fireShotParameters.direction == ProjectileDirection::Up);
      }
    }
  }


  SECTION("Player cannot shoot in certain states") {
    SECTION("Cannot shoot while climbing a ladder") {
      playerState.mState = PlayerState::ClimbingLadder;
      update(shootingInputState);
      CHECK(fireShotSpy.count() == 0);
    }

    SECTION("Cannot shoot while dieing") {
      playerState.mState = PlayerState::Dieing;
      update(shootingInputState);
      CHECK(fireShotSpy.count() == 0);
    }

    SECTION("Cannot shoot when dead") {
      playerState.mState = PlayerState::Dead;
      update(shootingInputState);
      CHECK(fireShotSpy.count() == 0);
    }
  }


  SECTION("Shot type depends on player's current weapon") {
    SECTION("Regular shot") {
      update(shootingInputState);
      CHECK(fireShotParameters.type == ProjectileType::PlayerRegularShot);
    }

    SECTION("Laser shot") {
      playerModel.switchToWeapon(data::WeaponType::Laser);

      update(shootingInputState);
      CHECK(fireShotParameters.type == ProjectileType::PlayerLaserShot);
    }

    SECTION("Rocket shot") {
      playerModel.switchToWeapon(data::WeaponType::Rocket);

      update(shootingInputState);
      CHECK(fireShotParameters.type == ProjectileType::PlayerRocketShot);
    }

    SECTION("Flame shot") {
      playerModel.switchToWeapon(data::WeaponType::FlameThrower);

      update(shootingInputState);
      CHECK(fireShotParameters.type == ProjectileType::PlayerFlameShot);
    }
  }


  SECTION("Shooting triggers appropriate sound") {
    CHECK(mockServiceProvicer.mLastTriggeredSoundId == boost::none);

    SECTION("Normal shot") {
      update(shootingInputState);
      CHECK(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      CHECK(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeNormalShot);
    }

    SECTION("Laser") {
      playerModel.switchToWeapon(data::WeaponType::Laser);

      update(shootingInputState);
      CHECK(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      CHECK(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeLaserShot);
    }

    SECTION("Rocket launcher") {
      playerModel.switchToWeapon(data::WeaponType::Rocket);

      // The rocket launcher also uses the normal shot sound
      update(shootingInputState);
      CHECK(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      CHECK(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeNormalShot);
    }

    SECTION("Flame thrower") {
      playerModel.switchToWeapon(data::WeaponType::FlameThrower);

      update(shootingInputState);
      CHECK(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      CHECK(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::FlameThrowerShot);
    }

    SECTION("Last shot before ammo depletion still uses appropriate sound") {
      playerModel.switchToWeapon(data::WeaponType::Laser);
      playerModel.setAmmo(1);

      update(shootingInputState);

      CHECK(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeLaserShot);
    }
  }


  const auto fireOneShot = [
    &attackSystem, &shootingInputState, &defaultInputState, &update
  ]() {
    update(shootingInputState);
    update(defaultInputState);
  };

  SECTION("Ammo consumption for non-regular weapons works") {
    SECTION("Normal shot doesn't consume ammo") {
      playerModel.setAmmo(24);
      fireOneShot();
      CHECK(playerModel.ammo() == 24);
    }

    SECTION("Laser consumes 1 unit of ammo per shot") {
      playerModel.switchToWeapon(data::WeaponType::Laser);
      playerModel.setAmmo(10);

      fireOneShot();
      CHECK(playerModel.ammo() == 9);
    }

    SECTION("Rocket launcher consumes 1 unit of ammo per shot") {
      playerModel.switchToWeapon(data::WeaponType::Rocket);
      playerModel.setAmmo(10);

      fireOneShot();
      CHECK(playerModel.ammo() == 9);
    }

    SECTION("Flame thrower consumes 1 unit of ammo per shot") {
      playerModel.switchToWeapon(data::WeaponType::FlameThrower);
      playerModel.setAmmo(10);

      fireOneShot();
      CHECK(playerModel.ammo() == 9);
    }

    SECTION("Multiple shots consume several units of ammo") {
      playerModel.switchToWeapon(data::WeaponType::Laser);
      playerModel.setAmmo(20);

      const auto shotsToFire = 15;
      for (int i = 0; i < shotsToFire; ++i) {
        fireOneShot();
      }

      CHECK(playerModel.ammo() == 20 - shotsToFire);
    }

    SECTION("Depleting ammo switches back to normal weapon") {
      playerModel.switchToWeapon(data::WeaponType::Rocket);
      playerModel.setAmmo(1);

      fireOneShot();

      CHECK(playerModel.weapon() == data::WeaponType::Normal);
      CHECK(playerModel.ammo() == playerModel.currentMaxAmmo());
    }
  }


  SECTION("Player cannot fire while in 'interacting' state") {
    playerState.mIsInteracting = true;
    update(shootingInputState);
    CHECK(fireShotSpy.count() == 0);
  }


  SECTION("Player fires only once when holding down fire button") {
    attackSystem.buttonStateChanged(shootingInputState);
    attackSystem.update();
    CHECK(fireShotSpy.count() == 1);

    for (int i = 0; i < 10; ++i) {
      attackSystem.update();
    }

    CHECK(fireShotSpy.count() == 1);
  }


  SECTION("Player fires continuously every other frame when owning rapid fire buff") {
    playerModel.giveItem(data::InventoryItemType::RapidFire);

    attackSystem.buttonStateChanged(shootingInputState);
    attackSystem.update();
    CHECK(fireShotSpy.count() == 1);

    attackSystem.update();
    attackSystem.update();
    CHECK(fireShotSpy.count() == 2);

    attackSystem.update();
    CHECK(fireShotSpy.count() == 2);

    attackSystem.update();
    CHECK(fireShotSpy.count() == 3);
  }

  SECTION("Firing stops when rapid fire is taken away") {
    playerModel.giveItem(data::InventoryItemType::RapidFire);

    attackSystem.buttonStateChanged(shootingInputState);
    for (int i = 0; i < 700; ++i) {
      attackSystem.update();
    }
    CHECK(fireShotSpy.count() == 350);

    playerModel.removeItem(data::InventoryItemType::RapidFire);

    for (int i = 0; i < 2; ++i) {
      attackSystem.update();
    }
    CHECK(fireShotSpy.count() == 350);
  }
}
