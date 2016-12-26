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

#include "debugging_system.hpp"

#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "sdl_utils/rect_tools.hpp"

namespace ex = entityx;


namespace rigel { namespace engine {

using namespace data;
using namespace engine::components;


DebuggingSystem::DebuggingSystem(
  SDL_Renderer* pRenderer,
  base::Vector* pScrollOffset
)
  : mpRenderer(pRenderer)
  , mpScrollOffset(pScrollOffset)
{
}


void DebuggingSystem::toggleBoundingBoxDisplay() {
  mShowBoundingBoxes = !mShowBoundingBoxes;
}


void DebuggingSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  if (mShowBoundingBoxes) {
    es.each<WorldPosition, BoundingBox>(
      [this](ex::Entity, const WorldPosition& pos, const BoundingBox& bbox) {
        const auto worldToScreenPx =
          tileVectorToPixelVector(*mpScrollOffset);

        const auto worldSpaceBox = engine::toWorldSpace(bbox, pos);
        const auto boxInPixels = BoundingBox{
          tileVectorToPixelVector(worldSpaceBox.topLeft) - worldToScreenPx,
          tileExtentsToPixelExtents(worldSpaceBox.size)
        };

        sdl_utils::drawRectangle(mpRenderer, boxInPixels, 255, 0, 0);
      });
  }
}

}}
