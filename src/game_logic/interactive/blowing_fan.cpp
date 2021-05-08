/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "blowing_fan.hpp"

#include "common/game_service_provider.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

const int FAN_ANIM_SEQUENCE[61] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 0, 1, 2, 3, 0,
  1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
  0, 2, 0, 2, 0, 2, 0, 2, 0
};


const int FAN_THREADS_ANIM_SEQUENCE[61] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 3, 2, 3, 2,
  3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2,
  3, 2, 3, 2, 3, 2, 3, 2, 3
};


constexpr int THREAD_ANIM_BASE_FRAME = 4;

}


void BlowingFan::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine::components;

  const auto& playerPos = s.mpPlayer->position();
  const auto& position = *entity.component<WorldPosition>();
  auto& spriteFrames = entity.component<Sprite>()->mFramesToRender;

  auto playerInHorizontalRange = [&]() {
    return position.x <= playerPos.x && position.x + 5 > playerPos.x;
  };

  auto playerInRange = [&]() {
    // clang-format off
    return
      playerInHorizontalRange() &&
      position.y > playerPos.y &&
      playerPos.y + 25 > position.y;
    // clang-format on
  };


  // Update fan spinning
  if (mState == State::SpeedingUp) {
    ++mStep;
    if (mStep == 60) {
      mState = State::SlowingDown;
    }
  } else {
    --mStep;
    if (mStep == 0) {
      mState = State::SpeedingUp;
    }
  }

  // Update animation & sound
  spriteFrames[0] = FAN_ANIM_SEQUENCE[mStep];
  spriteFrames[1] = FAN_THREADS_ANIM_SEQUENCE[mStep] + THREAD_ANIM_BASE_FRAME;
  if (spriteFrames[0] == 2 && isOnScreen) {
    d.mpServiceProvider->playSound(data::SoundId::Swoosh);
  }

  // Update attaching player
  if (mStep > 24 && playerInRange() && !s.mpPlayer->isDead()) {
    s.mpPlayer->beginBeingPushedByFan();
    mIsPushingPlayer = true;

    // clang-format off
    if (
      spriteFrames[0] == 3 ||
      playerPos.y + 24 == position.y ||
      playerPos.y + 25 == position.y)
    // clang-format on
    {
      d.mpServiceProvider->playSound(data::SoundId::Swoosh);
    }
  }

  const auto playerHasLeftRange =
    !playerInHorizontalRange() || position.y > playerPos.y + 25;
  if (mIsPushingPlayer && (mStep < 25 || playerHasLeftRange)) {
    s.mpPlayer->endBeingPushedByFan();
    mIsPushingPlayer = false;
  }
}

}
