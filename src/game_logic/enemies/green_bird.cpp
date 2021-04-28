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

#include "green_bird.hpp"

#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

void GreenBird::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::Orientation;
  using engine::components::WorldPosition;

  const auto& position = *entity.component<WorldPosition>();

  if (!entity.has_component<Orientation>())
  {
    const auto playerPosX = s.mpPlayer->orientedPosition().x;
    const auto initialOrientation =
      position.x <= playerPosX ? Orientation::Right : Orientation::Left;
    entity.assign<Orientation>(initialOrientation);
  }

  auto& orientation = *entity.component<Orientation>();

  const auto result = engine::moveHorizontally(
    *d.mpCollisionChecker,
    entity,
    engine::orientation::toMovement(orientation));
  if (result != engine::MovementResult::Completed)
  {
    orientation = engine::orientation::opposite(orientation);
  }
}

} // namespace rigel::game_logic::behaviors
