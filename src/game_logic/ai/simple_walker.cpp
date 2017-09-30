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

#include "engine/collision_checker.hpp"
#include "engine/sprite_tools.hpp"


namespace rigel { namespace game_logic { namespace ai {

namespace {

using namespace engine::components;
using namespace engine::orientation;


void updateAnimation(entityx::Entity entity, const Orientation orientation) {
  const auto startFrame = orientation == Orientation::Left ? 0 : 4;
  const auto endFrame = startFrame + 3;
  engine::startAnimationLoop(entity, 2, startFrame, endFrame);
}

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

  es.each<components::SimpleWalker, WorldPosition, Active>(
    [this, &playerPosition](
      entityx::Entity entity,
      components::SimpleWalker& state,
      WorldPosition& position,
      const Active&
    ) {
      if (!state.mOrientation) {
        state.mOrientation = position.x < playerPosition.x
          ? Orientation::Right
          : Orientation::Left;
        updateAnimation(entity, *state.mOrientation);
      }

      if (mIsOddFrame) {
        return;
      }

      auto& orientation = *state.mOrientation;
      const auto walkedSuccessfully = mpCollisionChecker->walkEntity(
        entity, toMovement(orientation));
      if (!walkedSuccessfully) {
        orientation = opposite(orientation);
        updateAnimation(entity, orientation);
      }
    });

  mIsOddFrame = !mIsOddFrame;
}

}}}
