/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "respawn_checkpoint.hpp"

#include "common/global.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::interaction
{

namespace
{

constexpr auto ACTIVATION_COUNTDOWN = 14;
constexpr auto PERFORM_CHECKPOINT_TIME = 9;

void turnIntoPassiveCheckpoint(entityx::Entity entity)
{
  entity.remove<components::BehaviorController>();
  entity.remove<engine::components::BoundingBox>();
  engine::startAnimationLoop(entity, 1, 5, 8);
}

} // namespace


void RespawnCheckpoint::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  auto& sprite = *entity.component<Sprite>();

  if (!mInitialized)
  {
    // Special case: If a respawn checkpoint is already visible on screen
    // when the level is loaded, it will immediately go into its "active"
    // state, and can't be triggered by the player anymore. This is
    // presumably because restoring from the checkpoint would be kind of
    // equivalent to restarting the level if the checkpoint is already
    // visible at the location where the player spawns.
    if (engine::isOnScreen(entity))
    {
      turnIntoPassiveCheckpoint(entity);
      return;
    }

    mInitialized = true;
  }

  if (!mActivationCountdown)
  {
    //
    // Collision check
    //
    const auto worldSpacePlayerBounds = s.mpPlayer->worldSpaceCollisionBox();
    const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
    if (worldSpaceBbox.intersects(worldSpacePlayerBounds))
    {
      mActivationCountdown = ACTIVATION_COUNTDOWN;
    }
  }
  else
  {
    //
    // Activation sequence
    //
    auto& countdown = *mActivationCountdown;
    if (countdown > 0)
    {
      // Part 1: Just flash white, after a few frames, trigger the actual
      // checkpoint to be made. Keep counting down, once the countdown hits
      // 0, the animation starts.
      --countdown;
      if (countdown % 2 == 0)
      {
        sprite.flashWhite();
      }

      if (countdown == PERFORM_CHECKPOINT_TIME)
      {
        d.mpEvents->emit(rigel::events::CheckPointActivated{position});
      }
    }
    else
    {
      // Part 2: Animate the checkpoint rising up, then switch to a loop
      // once that's done. This part wouldn't be necessary if we had a way
      // to express a combined animation sequence/loop, where the looped
      // part could be separate from a "startup" sequence.
      ++sprite.mFramesToRender[0];
      if (sprite.mFramesToRender[0] == 5)
      {
        turnIntoPassiveCheckpoint(entity);
      }
    }
  }
}

} // namespace rigel::game_logic::interaction
