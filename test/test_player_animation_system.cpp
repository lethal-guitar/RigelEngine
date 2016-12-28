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

#include <data/player_data.hpp>
#include <engine/visual_components.hpp>
#include <game_logic/player/animation_system.hpp>
#include <game_logic/player_movement_system.hpp>
#include <game_mode.hpp>


using namespace std;

using namespace rigel;
using namespace game_logic;

using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::PlayerControlled;
using game_logic::player::Orientation;
using game_logic::player::PlayerState;

namespace ex = entityx;


TEST_CASE("Player animation system works as expected") {
  ex::EntityX entityx;
  auto player = entityx.entities.create();
  player.assign<WorldPosition>(0, 0);

  player.assign<Sprite>();
  initializePlayerEntity(player, false);

  MockServiceProvider mockServiceProvicer;
  player::AnimationSystem animationSystem{
    player, &mockServiceProvicer, nullptr};

  auto& playerState = *player.component<PlayerControlled>().get();
  auto& playerSprite = *player.component<Sprite>().get();
  playerSprite.mFramesToRender.push_back(0);

  const auto updateFrames = [&animationSystem, &entityx](const int frames) {
    for (int i = 0; i < frames; ++i) {
      const auto dt = engine::gameFramesToTime(1);
      animationSystem.update(entityx.entities, entityx.events, dt);
    }
  };

  const auto update = [&animationSystem, &entityx](const ex::TimeDelta dt) {
    animationSystem.update(entityx.entities, entityx.events, dt);
  };


  SECTION("Death animation") {
    playerState.mState = PlayerState::Dieing;

    update(0.0);
    CHECK(playerSprite.mFramesToRender[0] == 0);

    updateFrames(1); // 1
    CHECK(playerSprite.mFramesToRender[0] == 29);

    updateFrames(1); // 2
    CHECK(playerSprite.mFramesToRender[0] == 29);

    updateFrames(3); // 5
    CHECK(playerSprite.mFramesToRender[0] == 30);

    updateFrames(1); // 6
    CHECK(playerSprite.mFramesToRender[0] == 31);

    updateFrames(1); // 7
    CHECK(playerSprite.mFramesToRender[0] == 32);

    updateFrames(1); // 8
    CHECK(playerSprite.mFramesToRender[0] == 32);

    updateFrames(8); // 16
    CHECK(playerSprite.mShow == true);

    updateFrames(1); // 17
    CHECK(playerSprite.mShow == false);
    CHECK(playerState.mState == PlayerState::Dieing);

    updateFrames(24); // 41
    CHECK(playerState.mState == PlayerState::Dieing);

    updateFrames(1); // 42
    CHECK(playerState.mState == PlayerState::Dead);
  }


  SECTION("Orientation change updates animation frame") {
    playerState.mOrientation = Orientation::Left;
    playerState.mState = PlayerState::LookingUp;
    updateFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 16);

    playerState.mOrientation = Orientation::Right;
    updateFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 16 + 39);
  }
}
