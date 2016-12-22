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
#include <base/warnings.hpp>
#include <data/player_data.hpp>
#include <game_logic/player/attack_system.hpp>
#include <game_logic/player_control_system.hpp>
#include <game_mode.hpp>

RIGEL_DISABLE_WARNINGS
#include <atria/testing/spies.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
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
  base::Point<float> directionVector;
};


struct MockServiceProvider : public rigel::IGameServiceProvider {
  void fadeOutScreen() override {}
  void fadeInScreen() override {}

  void playSound(data::SoundId id) override {
    mLastTriggeredSoundId = id;
  }

  void playMusic(const std::string&) override {}
  void stopMusic() override {}
  void scheduleNewGameStart(int, data::Difficulty) override {}
  void scheduleEnterMainMenu() override {}
  void scheduleGameQuit() override {}
  bool isShareWareVersion() const override { return false; }

  boost::optional<data::SoundId> mLastTriggeredSoundId;
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
    const base::Point<float>& directionVector
  ) {
    fireShotParameters.type = type;
    fireShotParameters.position = position;
    fireShotParameters.directionVector = directionVector;
  });

  MockServiceProvider mockServiceProvicer;
  entityx.systems.add<player::AttackSystem>(
    player,
    &playerModel,
    &mockServiceProvicer,
    fireShotSpy);
  entityx.systems.configure();
  auto& attackSystem = *entityx.systems.system<player::AttackSystem>();

  const auto defaultInputState = PlayerInputState{};
  auto shootingInputState = PlayerInputState{};
  shootingInputState.mShooting = true;

  REQUIRE(player.has_component<PlayerControlled>());
  auto& playerState = *player.component<PlayerControlled>().get();
  auto& playerPosition = *player.component<WorldPosition>().get();

  const auto updateWithInput = [&attackSystem, &entityx](
    const PlayerInputState& inputState,
    const ex::TimeDelta timeDelta = 10.0
  ) {
    attackSystem.setInputState(inputState);
    entityx.systems.update<player::AttackSystem>(timeDelta);
  };


  SECTION("Pressing fire button triggers only one shot") {
    updateWithInput(defaultInputState);
    REQUIRE(fireShotSpy.count() == 0);

    updateWithInput(shootingInputState);
    REQUIRE(fireShotSpy.count() == 1);

    // holding the fire button doesn't trigger another shot
    updateWithInput(shootingInputState);
    REQUIRE(fireShotSpy.count() == 1);

    SECTION("Releasing fire button allows shooting again on next press") {
      updateWithInput(defaultInputState);

      updateWithInput(shootingInputState);
      REQUIRE(fireShotSpy.count() == 2);
    }
  }


  SECTION("Shot position and direction depend on player orientation and state") {
    SECTION("Standing") {
      SECTION("Facing right") {
        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{3, -2};
        const auto expectedVector = base::Point<float>{1.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -2};
        const auto expectedVector = base::Point<float>{-1.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }

      SECTION("Player position offset") {
        playerPosition.x += 4;

        SECTION("Facing right") {
          updateWithInput(shootingInputState);
          REQUIRE(fireShotParameters.position.x == 3 + 4);
        }

        SECTION("Facing left") {
          playerState.mOrientation = player::Orientation::Left;

          updateWithInput(shootingInputState);
          REQUIRE(fireShotParameters.position.x == -1 + 4);
        }
      }
    }

    SECTION("Crouching") {
      playerState.mState = PlayerState::Crouching;

      SECTION("Facing right") {
        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{3, -1};
        const auto expectedVector = base::Point<float>{1.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -1};
        const auto expectedVector = base::Point<float>{-1.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }
    }

    SECTION("Looking up") {
      playerState.mState = PlayerState::LookingUp;

      SECTION("Facing right") {
        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{2, -5};
        const auto expectedVector = base::Point<float>{0.0f, -1.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{0, -5};
        const auto expectedVector = base::Point<float>{0.0f, -1.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.directionVector == expectedVector);
      }
    }
  }


  SECTION("Player cannot shoot in certain states") {
    SECTION("Cannot shoot while climbing a ladder") {
      playerState.mState = PlayerState::ClimbingLadder;
      updateWithInput(shootingInputState);
      REQUIRE(fireShotSpy.count() == 0);
    }

    SECTION("Cannot shoot while dieing") {
      playerState.mState = PlayerState::Dieing;
      updateWithInput(shootingInputState);
      REQUIRE(fireShotSpy.count() == 0);
    }

    SECTION("Cannot shoot when dead") {
      playerState.mState = PlayerState::Dead;
      updateWithInput(shootingInputState);
      REQUIRE(fireShotSpy.count() == 0);
    }
  }


  SECTION("Shot type depends on player's current weapon") {
    SECTION("Regular shot") {
      updateWithInput(shootingInputState);
      REQUIRE(fireShotParameters.type == ProjectileType::PlayerRegularShot);
    }

    SECTION("Laser shot") {
      playerModel.mWeapon = data::WeaponType::Laser;

      updateWithInput(shootingInputState);
      REQUIRE(fireShotParameters.type == ProjectileType::PlayerLaserShot);
    }

    SECTION("Rocket shot") {
      playerModel.mWeapon = data::WeaponType::Rocket;

      updateWithInput(shootingInputState);
      REQUIRE(fireShotParameters.type == ProjectileType::PlayerRocketShot);
    }

    SECTION("Flame shot") {
      playerModel.mWeapon = data::WeaponType::FlameThrower;

      updateWithInput(shootingInputState);
      REQUIRE(fireShotParameters.type == ProjectileType::PlayerFlameShot);
    }
  }


  SECTION("Shooting triggers appropriate sound") {
    REQUIRE(mockServiceProvicer.mLastTriggeredSoundId == boost::none);

    SECTION("Normal shot") {
      updateWithInput(shootingInputState);
      REQUIRE(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      REQUIRE(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeNormalShot);
    }

    SECTION("Laser") {
      playerModel.mWeapon = data::WeaponType::Laser;

      updateWithInput(shootingInputState);
      REQUIRE(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      REQUIRE(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeLaserShot);
    }

    SECTION("Rocket launcher") {
      playerModel.mWeapon = data::WeaponType::Rocket;

      // The rocket launcher also uses the normal shot sound
      updateWithInput(shootingInputState);
      REQUIRE(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      REQUIRE(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeNormalShot);
    }

    SECTION("Flame thrower") {
      playerModel.mWeapon = data::WeaponType::FlameThrower;

      updateWithInput(shootingInputState);
      REQUIRE(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
      REQUIRE(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::FlameThrowerShot);
    }

    SECTION("Last shot before ammo depletion still uses appropriate sound") {
      playerModel.mWeapon = data::WeaponType::Laser;
      playerModel.mAmmo = 1;

      updateWithInput(shootingInputState);

      REQUIRE(
          *mockServiceProvicer.mLastTriggeredSoundId ==
          data::SoundId::DukeLaserShot);
    }
  }


  const auto fireOneShot = [
    &updateWithInput, &shootingInputState, &defaultInputState
  ]() {
    updateWithInput(shootingInputState);
    updateWithInput(defaultInputState);
  };

  SECTION("Ammo consumption for non-regular weapons works") {
    SECTION("Normal shot doesn't consume ammo") {
      playerModel.mAmmo = 42;
      fireOneShot();
      REQUIRE(playerModel.mAmmo == 42);
    }

    SECTION("Laser consumes 1 unit of ammo per shot") {
      playerModel.mWeapon = data::WeaponType::Laser;
      playerModel.mAmmo = 10;

      fireOneShot();
      REQUIRE(playerModel.mAmmo == 9);
    }

    SECTION("Rocket launcher consumes 1 unit of ammo per shot") {
      playerModel.mWeapon = data::WeaponType::Rocket;
      playerModel.mAmmo = 10;

      fireOneShot();
      REQUIRE(playerModel.mAmmo == 9);
    }

    SECTION("Flame thrower consumes 1 unit of ammo per shot") {
      playerModel.mWeapon = data::WeaponType::FlameThrower;
      playerModel.mAmmo = 10;

      fireOneShot();
      REQUIRE(playerModel.mAmmo == 9);
    }

    SECTION("Multiple shots consume several units of ammo") {
      playerModel.mWeapon = data::WeaponType::Laser;
      playerModel.mAmmo = 20;

      const auto shotsToFire = 15;
      for (int i = 0; i < shotsToFire; ++i) {
        fireOneShot();
      }

      REQUIRE(playerModel.mAmmo == 20 - shotsToFire);
    }

    SECTION("Depleting ammo switches back to normal weapon") {
      playerModel.mWeapon = data::WeaponType::Rocket;
      playerModel.mAmmo = 1;

      fireOneShot();

      REQUIRE(playerModel.mWeapon == data::WeaponType::Normal);
      REQUIRE(playerModel.mAmmo == playerModel.currentMaxAmmo());
    }
  }
}
