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

#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/player/components.hpp"

#include "global_level_events.hpp"


namespace rigel { namespace game_logic { namespace interaction {

namespace {

constexpr auto ACTIVATION_COUNTDOWN = 14;
constexpr auto PERFORM_CHECKPOINT_TIME = 9;

void turnIntoPassiveCheckpoint(entityx::Entity entity) {
  entity.remove<components::RespawnCheckpoint>();
  entity.remove<engine::components::BoundingBox>();
  engine::startAnimationLoop(entity, 1, 5, 8);
}

}


RespawnCheckpointSystem::RespawnCheckpointSystem(
  entityx::Entity player,
  entityx::EventManager* pEvents
)
  : mPlayer(player)
  , mpEvents(pEvents)
{
}


void RespawnCheckpointSystem::update(entityx::EntityManager& es) {
  using namespace engine::components;
  using components::RespawnCheckpoint;

  const auto& playerBox = *mPlayer.component<BoundingBox>();
  const auto& playerPos = *mPlayer.component<WorldPosition>();
  const auto worldSpacePlayerBounds =
    engine::toWorldSpace(playerBox, playerPos);

  es.each<RespawnCheckpoint, WorldPosition, BoundingBox, Sprite>(
    [this, &worldSpacePlayerBounds](
      entityx::Entity entity,
      RespawnCheckpoint& state,
      const WorldPosition& position,
      const BoundingBox& bbox,
      Sprite& sprite
    ) {
      if (!state.mInitialized) {
        // Special case: If a respawn checkpoint is already visible on screen
        // when the level is loaded, it will immediately go into its "active"
        // state, and can't be triggered by the player anymore. This is
        // presumably because restoring from the checkpoint would be kind of
        // equivalent to restarting the level if the checkpoint is already
        // visible at the location where the player spawns.
        if (engine::isOnScreen(entity)) {
          turnIntoPassiveCheckpoint(entity);
          return;
        }

        state.mInitialized = true;
      }

      if (!state.mActivationCountdown) {
        //
        // Collision check
        //
        const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
        if (worldSpaceBbox.intersects(worldSpacePlayerBounds)) {
          state.mActivationCountdown = ACTIVATION_COUNTDOWN;
        }
      } else {
        //
        // Activation sequence
        //
        auto& countdown = *state.mActivationCountdown;
        if (countdown > 0) {
          // Part 1: Just flash white, after a few frames, trigger the actual
          // checkpoint to be made. Keep counting down, once the countdown hits
          // 0, the animation starts.
          --countdown;
          if (countdown % 2 == 0) {
            sprite.flashWhite();
          }

          if (countdown == PERFORM_CHECKPOINT_TIME) {
            mpEvents->emit(rigel::events::CheckPointActivated{position});
          }
        } else {
          // Part 2: Animate the checkpoint rising up, then switch to a loop
          // once that's done. This part wouldn't be necessary if we had a way
          // to express a combined animation sequence/loop, where the looped
          // part could be separate from a "startup" sequence.
          ++sprite.mFramesToRender[0];
          if (sprite.mFramesToRender[0] == 5) {
            turnIntoPassiveCheckpoint(entity);
          }
        }
      }
    });
}

}}}
