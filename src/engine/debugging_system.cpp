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

#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/damage_components.hpp"

namespace ex = entityx;


namespace rigel { namespace engine {

using namespace data;
using namespace engine::components;


namespace {

base::Color colorForEntity(entityx::Entity entity) {
  const auto isPlayerDamaging =
    entity.has_component<game_logic::components::PlayerDamaging>();
  const auto isSolidBody =
    entity.has_component<engine::components::SolidBody>();

  if (isPlayerDamaging) {
    return {255, 0, 0, 255};
  } else if (isSolidBody) {
    return {255, 255, 0, 255};
  } else {
    return {0, 255, 0, 255};
  }
}

}


DebuggingSystem::DebuggingSystem(
  engine::Renderer* pRenderer,
  base::Vector* pScrollOffset,
  data::map::Map* pMap
)
  : mpRenderer(pRenderer)
  , mpScrollOffset(pScrollOffset)
  , mpMap(pMap)
{
}


void DebuggingSystem::toggleBoundingBoxDisplay() {
  mShowBoundingBoxes = !mShowBoundingBoxes;
}


void DebuggingSystem::toggleWorldCollisionDataDisplay() {
  mShowWorldCollisionData = !mShowWorldCollisionData;
}


void DebuggingSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  if (mShowWorldCollisionData) {
    const auto drawColor = base::Color{255, 255, 0, 255};

    for (int y=0; y<GameTraits::mapViewPortHeightTiles; ++y) {
      for (int x=0; x<GameTraits::mapViewPortWidthTiles; ++x) {
        const auto col = x + mpScrollOffset->x;
        const auto row = y + mpScrollOffset->y;
        if (col >= mpMap->width() || row >= mpMap->height()) {
          continue;
        }

        const auto collisionData = mpMap->collisionData(col, row);
        const auto topLeft = tileVectorToPixelVector({x, y});
        const auto bottomRight = tileVectorToPixelVector({x + 1, y + 1});
        const auto left = topLeft.x;
        const auto top = topLeft.y;
        const auto right = bottomRight.x;
        const auto bottom = bottomRight.y;

        // TODO: Implement this using SolidEdge matching,
        // then remove these functions (isSolidXXX) from CollisionData
        if (collisionData.isSolidTop()) {
          mpRenderer->drawLine(left, top, right, top, drawColor);
        }
        if (collisionData.isSolidRight()) {
          mpRenderer->drawLine(right, top, right, bottom, drawColor);
        }
        if (collisionData.isSolidBottom()) {
          mpRenderer->drawLine(left, bottom, right, bottom, drawColor);
        }
        if (collisionData.isSolidLeft()) {
          mpRenderer->drawLine(left, top, left, bottom, drawColor);
        }
      }
    }
  }

  if (mShowBoundingBoxes) {
    es.each<WorldPosition, BoundingBox>(
      [this](
        ex::Entity entity,
        const WorldPosition& pos,
        const BoundingBox& bbox
      ) {
        const auto worldToScreenPx = tileVectorToPixelVector(*mpScrollOffset);
        const auto worldSpaceBox = engine::toWorldSpace(bbox, pos);
        const auto boxInPixels = BoundingBox{
          tileVectorToPixelVector(worldSpaceBox.topLeft) - worldToScreenPx,
          tileExtentsToPixelExtents(worldSpaceBox.size)
        };

        mpRenderer->drawRectangle(boxInPixels, colorForEntity(entity));
      });
  }
}

}}
