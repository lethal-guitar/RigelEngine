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

#include "security_camera.hpp"

#include "engine/visual_components.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

using engine::components::Sprite;
using engine::components::WorldPosition;


namespace
{

int determineFrameForCameraPosition(
  const WorldPosition& cameraPosition,
  const WorldPosition& playerPosition)
{
  const auto playerAbove = playerPosition.y < cameraPosition.y;
  const auto playerBelow = playerPosition.y > cameraPosition.y;
  const auto playerLeft = playerPosition.x < cameraPosition.x;
  const auto playerRight = playerPosition.x > cameraPosition.x;

  if (playerBelow)
  {
    return playerLeft ? 7 : (playerRight ? 1 : 0);
  }
  else if (playerAbove)
  {
    return playerLeft ? 5 : (playerRight ? 3 : 4);
  }
  else
  {
    return playerLeft ? 6 : (playerRight ? 2 : 0);
  }
}

} // namespace


void SecurityCamera::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  if (s.mpPlayer->isCloaked())
  {
    return;
  }

  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<Sprite>();

  const auto newFrame =
    determineFrameForCameraPosition(position, s.mpPlayer->position());
  sprite.mFramesToRender[0] = newFrame;
}

} // namespace rigel::game_logic::behaviors
