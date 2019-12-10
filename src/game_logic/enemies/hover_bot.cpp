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

#include "hover_bot.hpp"

#include "base/match.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel::game_logic::ai {

using namespace components::detail;
using namespace engine::components;
using namespace engine::orientation;


namespace {

constexpr auto SPAWN_DELAY = 36;
const auto BOT_SPAWN_OFFSET = base::Vector{1, 0};

constexpr auto TELEPORT_ANIMATION_START_FRAME = 12;
constexpr auto TELEPORT_ANIMATION_END_FRAME =
  TELEPORT_ANIMATION_START_FRAME + 6;

} // namespace


HoverBotSystem::HoverBotSystem(
  entityx::Entity player,
  engine::CollisionChecker* pCollisionChecker,
  EntityFactory* pEntityFactory
)
  : mPlayer(player)
  , mpCollisionChecker(pCollisionChecker)
  , mpEntityFactory(pEntityFactory)
{
}


void HoverBotSystem::update(entityx::EntityManager& es) {
  // Spawn machines
  es.each<WorldPosition, components::HoverBotSpawnMachine, Active>(
    [this](
      entityx::Entity entity,
      WorldPosition& position,
      components::HoverBotSpawnMachine& state,
      const Active&
    ) {
      if (state.mSpawnsRemaining > 0) {
        ++state.mNextSpawnCountdown;
        if (state.mNextSpawnCountdown == SPAWN_DELAY) {
          state.mNextSpawnCountdown = 0;
          auto robot =
            mpEntityFactory->createActor(data::ActorID::Hoverbot, position + BOT_SPAWN_OFFSET);
          robot.assign<Active>();
        }
      }
    });


  // Hover bots
  es.each<WorldPosition, Sprite, components::HoverBot, Active>(
    [this](
      entityx::Entity entity,
      WorldPosition& position,
      Sprite& sprite,
      components::HoverBot& botState,
      const Active&
    ) {
      base::match(botState,
        [&](TeleportingIn& state) {
          // The teleportation sequence begins with a single frame of nothing,
          // followed by 7 frames of teleport animation. The enemy is not
          // damaging to the player during the 1st (empty) frame.
          // Afterwards, the robot waits for 9 more frames before starting to
          // move.

          if (state.mFramesElapsed == 1) {
            // start teleport animation
            sprite.mShow = true;
            engine::startAnimationLoop(
              entity,
              1,
              TELEPORT_ANIMATION_START_FRAME,
              TELEPORT_ANIMATION_END_FRAME);
            entity.assign<game_logic::components::PlayerDamaging>(1);
          } else if (state.mFramesElapsed == 8) {
            // stop teleport animation, draw the robot's body with a looping
            // animation
            engine::startAnimationLoop(entity, 1, 0, 5);

            // draw the robot's eye
            sprite.mFramesToRender.push_back(6);
            entity.assign<game_logic::components::AppearsOnRadar>();
          } else if (state.mFramesElapsed == 16) {
            botState = Moving{Orientation::Left};
            return; // We must'n access 'state' after this point
          }

          engine::synchronizeBoundingBoxToSprite(entity);
          ++state.mFramesElapsed;
        },


        [&](Moving& state) {
          engine::walk(*mpCollisionChecker, entity, state.mOrientation);

          // TODO: use wonky player position (orientation dependent)
          const auto& playerPosition = *mPlayer.component<WorldPosition>();
          const auto playerIsLeft = position.x > playerPosition.x;
          const auto playerIsRight = position.x < playerPosition.x;
          if (
            (state.mOrientation == Orientation::Left && playerIsRight) ||
            (state.mOrientation == Orientation::Right && playerIsLeft)
          ) {
            auto newState = Reorientation{opposite(state.mOrientation)};
            updateReorientation(newState, botState, sprite);
            botState = newState;
          }
        },


        [&](Reorientation& state) {
          updateReorientation(state, botState, sprite);
        });
    });

  mIsOddFrame = !mIsOddFrame;
}


void HoverBotSystem::updateReorientation(
  Reorientation& state,
  components::HoverBot& robotState,
  Sprite& sprite
) {
  if (mIsOddFrame) {
    ++state.mStep;
  }

  const auto eyePosition = state.mTargetOrientation == Orientation::Left
    ? 5 - state.mStep : state.mStep;
  sprite.mFramesToRender[1] = 6 + eyePosition;

  if (state.mStep == 5) {
    robotState = Moving{state.mTargetOrientation};
  }
}

}
