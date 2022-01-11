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
#include "data/map.hpp"
#include "renderer/renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::game_logic
{

class DebuggingSystem
{
public:
  DebuggingSystem(renderer::Renderer* pRenderer, data::map::Map* pMap);

  void toggleBoundingBoxDisplay();
  void toggleWorldCollisionDataDisplay();
  void toggleGridDisplay();

  void update(
    entityx::EntityManager& es,
    const base::Vec2& cameraPosition,
    const base::Extents& viewportSize,
    float interpolationFactor);

private:
  renderer::Renderer* mpRenderer;
  data::map::Map* mpMap;

  bool mShowBoundingBoxes = false;
  bool mShowWorldCollisionData = false;
  bool mShowGrid = false;
};

} // namespace rigel::game_logic
