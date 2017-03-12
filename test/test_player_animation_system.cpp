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

  auto& playerState = *player.component<PlayerControlled>();
  auto& playerSprite = *player.component<Sprite>();
  playerSprite.mFramesToRender.push_back(0);

  const auto runFrames = [&animationSystem, &entityx](const int frames) {
    for (int i = 0; i < frames; ++i) {
      animationSystem.update(entityx.entities, entityx.events, 0);
    }
  };

  SECTION("Death animation") {
    playerState.mState = PlayerState::Dieing;

    runFrames(1); // 1
    CHECK(playerSprite.mFramesToRender[0] == 29);

    runFrames(1); // 2
    CHECK(playerSprite.mFramesToRender[0] == 29);

    runFrames(3); // 5
    CHECK(playerSprite.mFramesToRender[0] == 30);

    runFrames(1); // 6
    CHECK(playerSprite.mFramesToRender[0] == 31);

    runFrames(1); // 7
    CHECK(playerSprite.mFramesToRender[0] == 32);

    runFrames(1); // 8
    CHECK(playerSprite.mFramesToRender[0] == 32);

    runFrames(8); // 16
    CHECK(playerSprite.mShow == true);

    runFrames(1); // 17
    CHECK(playerSprite.mShow == false);
    CHECK(playerState.mState == PlayerState::Dieing);

    runFrames(24); // 41
    CHECK(playerState.mState == PlayerState::Dieing);

    runFrames(1); // 42
    CHECK(playerState.mState == PlayerState::Dead);
  }


  SECTION("Orientation change updates animation frame") {
    playerState.mOrientation = Orientation::Left;
    playerState.mState = PlayerState::LookingUp;
    runFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 16);

    playerState.mOrientation = Orientation::Right;
    runFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 16 + 39);
  }


  SECTION("'is interacting' state is applied correctly") {
    playerState.mIsInteracting = true;
    runFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 33);

    playerState.mIsInteracting = false;
    runFrames(1);
    CHECK(playerSprite.mFramesToRender[0] == 0);
  }
}
