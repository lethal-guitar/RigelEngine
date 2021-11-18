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

#include "flame_thrower_bot.hpp"

#include "data/actor_ids.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic::behaviors
{

void FlameThrowerBot::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::Orientation;
  using engine::components::WorldPosition;

  if (const auto num = d.mpRandomGenerator->gen(); num == 0 || num == 128)
  {
    mFramesRemainingForFiring = 16;
  }

  if (mFramesRemainingForFiring > 0)
  {
    --mFramesRemainingForFiring;
    if (mFramesRemainingForFiring == 8)
    {
      const auto isFacingLeft =
        *entity.component<Orientation>() == Orientation::Left;
      const auto id = isFacingLeft ? data::ActorID::Flame_thrower_fire_LEFT
                                   : data::ActorID::Flame_thrower_fire_RIGHT;
      const auto xOffset = isFacingLeft ? -7 : 7;
      spawnOneShotSprite(
        *d.mpEntityFactory,
        id,
        *entity.component<WorldPosition>() + base::Vec2{xOffset, -3});
    }
  }
  else
  {
    // When moving up, we move at half speed (only on odd frames)
    if (
      mMovementDirection == MovementDirection::Up &&
      !s.mpPerFrameState->mIsOddFrame)
    {
      return;
    }

    const auto amount = mMovementDirection == MovementDirection::Up ? -1 : 1;
    const auto result =
      engine::moveVertically(*d.mpCollisionChecker, entity, amount);
    if (result != engine::MovementResult::Completed)
    {
      // Switch direction if we hit the floor/ceiling
      mMovementDirection = mMovementDirection == MovementDirection::Up
        ? MovementDirection::Down
        : MovementDirection::Up;
    }
  }
}

} // namespace rigel::game_logic::behaviors
