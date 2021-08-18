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

#include "snake.hpp"

#include "base/match.hpp"
#include "data/player_model.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

int GRAB_PLAYER_ANIMATION[] = {2, 3, 4, 5, 6};

}


void Snake::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace snake;
  using namespace engine::components;

  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];
  auto& orientation = *entity.component<Orientation>();
  auto& position = *entity.component<WorldPosition>();
  auto& playerPos = s.mpPlayer->position();

  auto destroySelf = false;

  auto movementValue = [&]() {
    return engine::orientation::toMovement(orientation);
  };

  auto walk = [&]() {
    const auto successful =
      engine::walk(*d.mpCollisionChecker, entity, orientation);
    if (!successful)
    {
      position.x += movementValue() * 2;
      orientation = engine::orientation::opposite(orientation);
    }
  };

  auto playerInReach = [&]() {
    // TODO: Extract a function for this? Also used by e.g. CeilingSucker
    const auto& bbox = *entity.component<engine::components::BoundingBox>();
    const auto worldBbox = engine::toWorldSpace(bbox, position);
    const auto touchingPlayer =
      worldBbox.intersects(s.mpPlayer->worldSpaceHitBox());

    if (!touchingPlayer)
    {
      return false;
    }

    const auto myX = position.x;
    const auto playerX = playerPos.x;

    // TODO: Can we do this distance check in a nicer way?
    const auto offset1 = 3 * movementValue();
    const auto offset2 = 2 * movementValue();

    const auto inReachHorizontally =
      myX + offset1 == playerX || myX + offset2 == playerX;
    const auto inReachVertically = position.y == playerPos.y;

    return inReachHorizontally && inReachVertically &&
      s.mpPlayer->isInRegularState();
  };

  auto walkWhilePlayerSwallowed = [&]() {
    if (s.mpPlayer->isDead())
    {
      triggerEffects(entity, *d.mpEntityManager);
      destroySelf = true;
      return;
    }

    s.mpPlayer->takeDamage(1);

    const auto fireButtonPressed =
      s.mpPerFrameState->mInput.mFire.mWasTriggered;
    if (!s.mpPlayer->isDead() && fireButtonPressed)
    {
      // Setting the player free again happens in the onKilled() handler.
      // This is to handle the edge case where the player kills the snake right
      // before being incapacitated, which would lead to a soft lock due to the
      // player never being set free if we didn't handle it in onKilled().

      // TODO: Is there a way we don't have to duplicate the score assignment/
      // event triggering here?
      const auto& shootable = *entity.component<components::Shootable>();
      d.mpEvents->emit(game_logic::events::ShootableKilled{entity, {}});
      s.mpPlayer->model().giveScore(shootable.mGivenScore);
      destroySelf = true;
      return;
    }

    animationFrame = 7 + (s.mpPerFrameState->mIsOddFrame ? 1 : 0);
    engine::synchronizeBoundingBoxToSprite(entity);

    if (s.mpPerFrameState->mIsOddFrame)
    {
      playerPos.x += movementValue();
      walk();
    }
  };


  base::match(
    mState,
    [&, this](const Walking&) {
      if (!s.mpPerFrameState->mIsOddFrame)
      {
        walk();
        animationFrame = position.x % 2;
      }

      if (playerInReach())
      {
        mState = GrabbingPlayer{};
      }
    },

    [&, this](GrabbingPlayer& state) {
      if (state.mFramesElapsed == 0)
      {
        s.mpPlayer->incapacitate(2);
        engine::startAnimationSequence(entity, GRAB_PLAYER_ANIMATION);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 6)
      {
        playerPos.x += 2;
        mState = SwallowedPlayer{};
        walkWhilePlayerSwallowed();
      }
    },

    [&, this](const SwallowedPlayer&) { walkWhilePlayerSwallowed(); });

  engine::synchronizeBoundingBoxToSprite(entity);

  if (destroySelf)
  {
    entity.destroy();
  }
}


void Snake::onKilled(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Point<float>&,
  entityx::Entity entity)
{
  if (s.mpPlayer->isIncapacitated())
  {
    s.mpPlayer->setFree();
  }
}

} // namespace rigel::game_logic::behaviors
