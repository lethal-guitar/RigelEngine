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
  WorldPosition position;
  base::Point<float> velocity;
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

  FireShotParameters fireShotParameters;
  auto fireShotSpy = atria::testing::spy([&fireShotParameters](
    const WorldPosition& position,
    const base::Point<float>& velocity
  ) {
    fireShotParameters.position = position;
    fireShotParameters.velocity = velocity;
  });

  MockServiceProvider mockServiceProvicer;
  entityx.systems.add<player::AttackSystem>(
    player, &mockServiceProvicer, fireShotSpy);
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
        const auto expectedVelocity = base::Point<float>{2.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -2};
        const auto expectedVelocity = base::Point<float>{-2.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
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
        const auto expectedVelocity = base::Point<float>{2.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{-1, -1};
        const auto expectedVelocity = base::Point<float>{-2.0f, 0.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
      }
    }

    SECTION("Looking up") {
      playerState.mState = PlayerState::LookingUp;

      SECTION("Facing right") {
        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{2, -5};
        const auto expectedVelocity = base::Point<float>{0.0f, -2.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
      }

      SECTION("Facing left") {
        playerState.mOrientation = player::Orientation::Left;

        updateWithInput(shootingInputState);

        const auto expectedPosition = WorldPosition{0, -5};
        const auto expectedVelocity = base::Point<float>{0.0f, -2.0f};
        REQUIRE(fireShotParameters.position == expectedPosition);
        REQUIRE(fireShotParameters.velocity == expectedVelocity);
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


  SECTION("Shooting triggers appropriate sound") {
    REQUIRE(mockServiceProvicer.mLastTriggeredSoundId == boost::none);

    updateWithInput(shootingInputState);

    REQUIRE(mockServiceProvicer.mLastTriggeredSoundId != boost::none);
    REQUIRE(
      *mockServiceProvicer.mLastTriggeredSoundId ==
      data::SoundId::DukeNormalShot);
  }
}
