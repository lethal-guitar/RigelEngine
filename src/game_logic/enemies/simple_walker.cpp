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

#include "engine/movement.hpp"
#include "engine/visual_components.hpp"


namespace rigel { namespace game_logic { namespace ai {

namespace {

using namespace engine::components;
using namespace engine::orientation;

} // namespace


SimpleWalkerSystem::SimpleWalkerSystem(
  entityx::Entity player,
  engine::CollisionChecker* pCollisionChecker
)
  : mPlayer(player)
  , mpCollisionChecker(pCollisionChecker)
{
}


void SimpleWalkerSystem::update(entityx::EntityManager& es) {
  const auto& playerPosition = *mPlayer.component<WorldPosition>();

  es.each<components::SimpleWalker, Sprite, WorldPosition, Active>(
    [this, &playerPosition](
      entityx::Entity entity,
      components::SimpleWalker& state,
      Sprite& sprite,
      WorldPosition& position,
      const Active&
    ) {
      if (!entity.has_component<Orientation>()) {
        const auto initialOrientation = position.x < playerPosition.x
          ? Orientation::Right
          : Orientation::Left;
        entity.assign<Orientation>(initialOrientation);
      }

      if (mIsOddFrame || state.mpConfig->mWalkAtFullSpeed) {
        auto& orientation = *entity.component<Orientation>();

        auto walkedSuccessfully = false;
        if (state.mpConfig->mWalkOnCeiling) {
          walkedSuccessfully =
            engine::walkOnCeiling(*mpCollisionChecker, entity, orientation);
        } else {
          walkedSuccessfully =
            engine::walk(*mpCollisionChecker, entity, orientation);
        }

        if (!walkedSuccessfully) {
          orientation = opposite(orientation);
        }

        auto& animationFrame = sprite.mFramesToRender[0];
        ++animationFrame;
        if (animationFrame == state.mpConfig->mAnimEnd + 1) {
          animationFrame = state.mpConfig->mAnimStart;
        }
      }
    });

  mIsOddFrame = !mIsOddFrame;
}

}}}
