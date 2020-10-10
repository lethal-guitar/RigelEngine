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

#include "simple_walker.hpp"

#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

using namespace engine::components;
using namespace engine::orientation;

} // namespace


void SimpleWalker::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity
) {
  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPosition = s.mpPlayer->orientedPosition();
  auto& sprite = *entity.component<Sprite>();

  if (!entity.has_component<Orientation>()) {
    const auto initialOrientation = position.x < playerPosition.x
      ? Orientation::Right
      : Orientation::Left;
    entity.assign<Orientation>(initialOrientation);
  }

  if (s.mpPerFrameState->mIsOddFrame || mpConfig->mWalkAtFullSpeed) {
    auto& orientation = *entity.component<Orientation>();

    auto walkedSuccessfully = false;
    if (mpConfig->mWalkOnCeiling) {
      walkedSuccessfully =
        engine::walkOnCeiling(*d.mpCollisionChecker, entity, orientation);
    } else {
      walkedSuccessfully =
        engine::walk(*d.mpCollisionChecker, entity, orientation);
    }

    if (!walkedSuccessfully) {
      orientation = opposite(orientation);
    }

    auto& animationFrame = sprite.mFramesToRender[0];
    ++animationFrame;
    if (animationFrame == mpConfig->mAnimEnd + 1) {
      animationFrame = mpConfig->mAnimStart;
    }
  }
}

}
