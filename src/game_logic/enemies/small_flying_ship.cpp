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

#include "small_flying_ship.hpp"

#include "data/player_model.hpp"
#include "engine/collision_checker.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

void SmallFlyingShip::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using engine::components::BoundingBox;
  using engine::components::WorldPosition;
  using game_logic::components::Shootable;

  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  auto isGroundAtOffset = [&](const int offset) {
    return d.mpCollisionChecker->isOnSolidGround(
      position + base::Vector{0, offset - 1}, {{}, {1, 1}});
  };

  auto findDistanceToGround = [&](const int maxDistance) -> std::optional<int> {
    for (int offset = 0; offset < maxDistance; ++offset)
    {
      if (isGroundAtOffset(offset))
      {
        return offset;
      }
    }

    return std::nullopt;
  };


  if (d.mpCollisionChecker->isTouchingLeftWall(position, bbox))
  {
    triggerEffects(entity, *d.mpEntityManager);
    s.mpPlayer->model().giveScore(entity.component<Shootable>()->mGivenScore);
    entity.destroy();
    return;
  }

  if (!mInitialHeight || *mInitialHeight == 0)
  {
    mInitialHeight = findDistanceToGround(15);
  }

  if (mInitialHeight)
  {
    const auto newHeight = findDistanceToGround(*mInitialHeight);
    if (newHeight)
    {
      --position.y;
    }
    else
    {
      if (!isGroundAtOffset(*mInitialHeight))
      {
        ++position.y;
      }
    }
  }
  else
  {
    if (!isGroundAtOffset(0))
    {
      ++position.y;
    }
  }

  --position.x;

  const auto stillOnScreen =
    isBboxOnScreen(s, engine::toWorldSpace(bbox, position));
  if (!stillOnScreen && position.x - 20 == s.mpPlayer->orientedPosition().x)
  {
    entity.destroy();
  }
}

} // namespace rigel::game_logic::behaviors
