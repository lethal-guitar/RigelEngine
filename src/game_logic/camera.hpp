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

#pragma once

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "game_logic/player/components.hpp"
#include "game_logic_common/input.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel::data::map
{
class Map;
}
namespace rigel::events
{
struct PlayerFiredShot;
}


namespace rigel::game_logic
{


class Player;

class Camera : public entityx::Receiver<Camera>
{
public:
  Camera(
    const Player* pPlayer,
    const data::map::Map& map,
    entityx::EventManager& eventManager);

  void synchronizeTo(const Camera& other);

  void update(const PlayerInput& input, const base::Size& viewportSize);
  void recenter(const base::Size& viewportSize);
  void centerViewOnPlayer();

  const base::Vec2& position() const;

  void receive(const rigel::events::PlayerFiredShot& event);

private:
  void updateManualScrolling(const PlayerInput& input);
  void updateAutomaticScrolling();
  void setPosition(base::Vec2 position);

  const Player* mpPlayer;
  const data::map::Map* mpMap;
  base::Vec2 mPosition;
  base::Size mViewportSize;
  int mManualScrollCooldown = 0;
};


inline const base::Vec2& Camera::position() const
{
  return mPosition;
}

} // namespace rigel::game_logic
