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

#include "eyeball_thrower.hpp"

#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"

RIGEL_DISABLE_WARNINGS
#include <atria/variant/match_boost.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace game_logic { namespace behaviors {

namespace {

constexpr auto EYEBALL_THROWER_WIDTH = 5;

const int GET_UP_ANIMATION_SEQUENCE[] = { 0, 0, 0, 0, 0, 1, 2, 3, 4, 5 };

}


void EyeballThrower::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine::components;
  using namespace eyeball_thrower;

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPos = s.mpPlayer->orientedPosition();

  auto& orientation = *entity.component<Orientation>();
  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];

  auto canShootAtPlayer = [&]() {
    // TODO: Extract helper function centerToCenterDistance()

    // [playerpos] Using orientation-independent position here
    const auto playerX = s.mpPlayer->position().x;
    const auto playerCenterX = playerX + PLAYER_WIDTH/2;
    const auto myCenterX = position.x + EYEBALL_THROWER_WIDTH/2;
    const auto centerToCenterDistance = std::abs(playerCenterX - myCenterX);

    const auto facingPlayer =
      (orientation == Orientation::Left && position.x > playerX) ||
      (orientation == Orientation::Right && position.x < playerX);
    const auto playerInRange =
      centerToCenterDistance > 9 &&
      centerToCenterDistance <= 14;

    return facingPlayer && playerInRange;
  };

  auto spawnEyeBall = [&]() {
    const auto offsetX = orientation == Orientation::Left ? 0 : 3;
    const auto movement = orientation == Orientation::Left
      ? SpriteMovement::FlyUpperLeft
      : SpriteMovement::FlyUpperRight;

    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      100,
      movement,
      position + base::Vector{offsetX, -6});
  };

  auto animateWalking = [&]() {
    animationFrame = animationFrame == 5 ? 6 : 5;
  };


  atria::variant::match(mState,
    [&, this](GettingUp& state) {
      if (state.mFramesElapsed == 0) {
        orientation = position.x <= playerPos.x
          ? Orientation::Right
          : Orientation::Left;
      } else if (state.mFramesElapsed == 1) {
        engine::startAnimationSequence(entity, GET_UP_ANIMATION_SEQUENCE);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 11) {
        mState = Walking{};
      }
    },

    [&, this](Walking& state) {
      ++mFramesElapsedInWalkingState;

      if (canShootAtPlayer()) {
        mState = Attacking{};
        return;
      }

      if (mFramesElapsedInWalkingState % 4 == 0) {
        animateWalking();

        const auto walkedSuccessfully =
          engine::walk(*d.mpCollisionChecker, entity, orientation);
        if (!walkedSuccessfully) {
          animationFrame = 1;
          mState = GettingUp{};
        }
      }
    },

    [&, this](Attacking& state) {
      // Animation sequence: 7, 7, 8, 8, 9, 9
      animationFrame = state.mFramesElapsed / 2 + 7;

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 4) {
        spawnEyeBall();
      } else if (state.mFramesElapsed == 6) {
        mState = Walking{};
      }
    }
  );
}

}}}
