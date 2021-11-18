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

#include "rocket_turret.hpp"

#include "base/math_tools.hpp"
#include "common/game_service_provider.hpp"
#include "engine/base_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr base::Vec2 OFFSET_BY_ORIENTATION[] = {{-2, -1}, {1, -2}, {2, -1}};


constexpr data::ActorID ACTOR_ID_BY_ORIENTATION[] = {
  data::ActorID::Enemy_rocket_left,
  data::ActorID::Enemy_rocket_up,
  data::ActorID::Enemy_rocket_right};


auto determineOrientation(
  const base::Vec2& position,
  const base::Vec2& playerPosition)
{
  using Orientation = behaviors::RocketTurret::Orientation;

  if (playerPosition.x + 3 <= position.x)
  {
    return Orientation::Left;
  }
  else if (playerPosition.x - 3 >= position.x)
  {
    return Orientation::Right;
  }
  else if (playerPosition.y <= position.y)
  {
    return Orientation::Top;
  }

  return Orientation::Left;
}

} // namespace


void RocketTurret::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::WorldPosition;

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPosition = s.mpPlayer->orientedPosition();

  if (mNeedsReorientation)
  {
    mOrientation = determineOrientation(position, playerPosition);
    mNeedsReorientation = false;
  }
  else
  {
    ++mNextShotCountdown;
    if (mNextShotCountdown >= 25)
    {
      mNextShotCountdown = 0;
      mNeedsReorientation = true;

      const auto orientationIndex = static_cast<int>(mOrientation);
      d.mpEntityFactory->spawnActor(
        ACTOR_ID_BY_ORIENTATION[orientationIndex],
        position + OFFSET_BY_ORIENTATION[orientationIndex]);
      d.mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
    }
  }

  auto& sprite = *entity.component<engine::components::Sprite>();
  sprite.mFramesToRender[0] = static_cast<int>(mOrientation);
  engine::synchronizeBoundingBoxToSprite(entity);
}

} // namespace rigel::game_logic::behaviors
