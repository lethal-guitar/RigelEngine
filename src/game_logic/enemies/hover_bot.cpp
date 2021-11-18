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
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr auto SPAWN_DELAY = 36;
const auto BOT_SPAWN_OFFSET = base::Vec2{1, 0};

constexpr auto TELEPORT_ANIMATION_START_FRAME = 12;
constexpr auto TELEPORT_ANIMATION_END_FRAME =
  TELEPORT_ANIMATION_START_FRAME + 6;

} // namespace


void HoverBotSpawnMachine::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using engine::components::WorldPosition;

  if (mSpawnsRemaining > 0)
  {
    ++mNextSpawnCountdown;
    if (mNextSpawnCountdown == SPAWN_DELAY)
    {
      mNextSpawnCountdown = 0;
      --mSpawnsRemaining;

      const auto& position = *entity.component<WorldPosition>();
      auto robot = d.mpEntityFactory->spawnActor(
        data::ActorID::Hoverbot, position + BOT_SPAWN_OFFSET);
      robot.assign<engine::components::Active>();
    }
  }
}


void HoverBot::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<engine::components::Sprite>();

  auto updateReorientation = [&](Reorientation& state) {
    if (s.mpPerFrameState->mIsOddFrame)
    {
      ++state.mStep;
    }

    const auto eyePosition = state.mTargetOrientation == Orientation::Left
      ? 5 - state.mStep
      : state.mStep;
    sprite.mFramesToRender[1] = 6 + eyePosition;

    if (state.mStep == 5)
    {
      mState = Moving{state.mTargetOrientation};
    }
  };


  base::match(
    mState,
    [&](TeleportingIn& state) {
      // The teleportation sequence begins with a single frame of nothing,
      // followed by 7 frames of teleport animation. The enemy is not
      // damaging to the player during the 1st (empty) frame.
      // Afterwards, the robot waits for 9 more frames before starting to
      // move.

      if (state.mFramesElapsed == 1)
      {
        // start teleport animation
        sprite.mShow = true;
        engine::startAnimationLoop(
          entity,
          1,
          TELEPORT_ANIMATION_START_FRAME,
          TELEPORT_ANIMATION_END_FRAME);
        entity.assign<game_logic::components::PlayerDamaging>(1);
      }
      else if (state.mFramesElapsed == 8)
      {
        // stop teleport animation, draw the robot's body with a looping
        // animation
        engine::startAnimationLoop(entity, 1, 0, 5);

        // draw the robot's eye
        sprite.mFramesToRender[1] = 6;
        entity.assign<game_logic::components::AppearsOnRadar>();
      }
      else if (state.mFramesElapsed == 16)
      {
        mState = Moving{Orientation::Left};
        return; // We must'n access 'state' after this point
      }

      engine::synchronizeBoundingBoxToSprite(entity);
      ++state.mFramesElapsed;
    },


    [&](Moving& state) {
      engine::walk(*d.mpCollisionChecker, entity, state.mOrientation);

      const auto& playerPosition = s.mpPlayer->orientedPosition();
      const auto playerIsLeft = position.x > playerPosition.x;
      const auto playerIsRight = position.x < playerPosition.x;
      if (
        (state.mOrientation == Orientation::Left && playerIsRight) ||
        (state.mOrientation == Orientation::Right && playerIsLeft))
      {
        auto newState =
          Reorientation{engine::orientation::opposite(state.mOrientation)};
        updateReorientation(newState);
        mState = newState;
      }
    },


    [&](Reorientation& state) { updateReorientation(state); });
}

} // namespace rigel::game_logic::behaviors
