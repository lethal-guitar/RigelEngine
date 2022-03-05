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
#include "engine/motion_smoothing.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"

namespace ex = entityx;


namespace rigel::game_logic
{

using namespace data;
using namespace engine::components;
using namespace std;
using data::map::SolidEdge;


namespace
{

struct SolidEdgeVisualizationInfo
{
  data::map::SolidEdge mEdge;
  tuple<int, int, int, int> mCoordinates;
};


base::Color colorForEntity(entityx::Entity entity)
{
  const auto isPlayerDamaging =
    entity.has_component<game_logic::components::PlayerDamaging>();
  const auto isSolidBody =
    entity.has_component<engine::components::SolidBody>();

  if (isPlayerDamaging)
  {
    return {255, 0, 0, 255};
  }
  else if (isSolidBody)
  {
    return {255, 255, 0, 255};
  }
  else
  {
    return {0, 255, 0, 255};
  }
}

} // namespace


DebuggingSystem::DebuggingSystem(
  renderer::Renderer* pRenderer,
  data::map::Map* pMap)
  : mpRenderer(pRenderer)
  , mpMap(pMap)
{
}


void DebuggingSystem::toggleBoundingBoxDisplay()
{
  mShowBoundingBoxes = !mShowBoundingBoxes;
}


void DebuggingSystem::toggleWorldCollisionDataDisplay()
{
  mShowWorldCollisionData = !mShowWorldCollisionData;
}


void DebuggingSystem::toggleGridDisplay()
{
  mShowGrid = !mShowGrid;
}


void DebuggingSystem::update(
  ex::EntityManager& es,
  const base::Vec2& cameraPosition,
  const base::Extents& viewportSize,
  const float interpolationFactor)
{
  if (mShowWorldCollisionData)
  {
    const auto drawColor = base::Color{255, 255, 0, 255};

    for (int y = 0; y < viewportSize.height; ++y)
    {
      for (int x = 0; x < viewportSize.width; ++x)
      {
        const auto col = x + cameraPosition.x;
        const auto row = y + cameraPosition.y;
        if (col >= mpMap->width() || row >= mpMap->height())
        {
          continue;
        }

        const auto collisionData = mpMap->collisionData(col, row);
        const auto topLeft = tileVectorToPixelVector({x, y});
        const auto bottomRight = tileVectorToPixelVector({x + 1, y + 1});
        const auto left = topLeft.x;
        const auto top = topLeft.y;
        const auto right = bottomRight.x;
        const auto bottom = bottomRight.y;

        const SolidEdgeVisualizationInfo visualizationInfos[] = {
          {SolidEdge::top(), make_tuple(left, top, right, top)},
          {SolidEdge::right(), make_tuple(right, top, right, bottom)},
          {SolidEdge::bottom(), make_tuple(left, bottom, right, bottom)},
          {SolidEdge::left(), make_tuple(left, top, left, bottom)}};

        for (const auto& info : visualizationInfos)
        {
          if (collisionData.isSolidOn(info.mEdge))
          {
            int x1, y1, x2, y2;
            tie(x1, y1, x2, y2) = info.mCoordinates;
            mpRenderer->drawLine(x1, y1, x2, y2, drawColor);
          }
        }

        const auto isClimbable = mpMap->attributes(col, row).isClimbable();
        const auto isLadder = mpMap->attributes(col, row).isLadder();
        const auto isFlammable = mpMap->attributes(col, row).isFlammable();

        if (isClimbable)
        {
          auto tileBox = base::makeRect<int>(topLeft, bottomRight);
          mpRenderer->drawRectangle(tileBox, base::Color(255, 100, 255, 220));
        }

        if (isLadder)
        {
          auto tileBox = base::makeRect<int>(topLeft, bottomRight);
          mpRenderer->drawRectangle(tileBox, base::Color(0, 100, 255, 220));
        }

        if (isFlammable)
        {
          auto tileBox = base::makeRect<int>(topLeft, bottomRight);
          mpRenderer->drawRectangle(tileBox, base::Color(255, 127, 0, 220));
        }
      }
    }
  }

  if (mShowBoundingBoxes)
  {
    es.each<WorldPosition, BoundingBox>(
      [&](
        ex::Entity entity, const WorldPosition& pos, const BoundingBox& bbox) {
        const auto worldToScreenPx = tileVectorToPixelVector(cameraPosition);
        const auto worldSpaceBox = engine::toWorldSpace(bbox, pos);

        const auto topLeftCurrent = worldSpaceBox.topLeft;
        const auto topLeftPrevious = entity.has_component<InterpolateMotion>()
          ? engine::toWorldSpace(
              bbox, entity.component<InterpolateMotion>()->mPreviousPosition)
              .topLeft
          : topLeftCurrent;

        auto transform = [&](const base::Vec2& p) {
          return tileVectorToPixelVector(p) - worldToScreenPx;
        };

        const auto visualTopLeft = engine::lerpRounded(
          transform(topLeftPrevious),
          transform(topLeftCurrent),
          interpolationFactor);

        const auto boxInPixels = BoundingBox{
          visualTopLeft, tileExtentsToPixelExtents(worldSpaceBox.size)};

        mpRenderer->drawRectangle(boxInPixels, colorForEntity(entity));
      });

    es.each<game_logic::components::MapGeometryLink>(
      [&](
        ex::Entity entity,
        const game_logic::components::MapGeometryLink& link) {
        const auto worldToScreenPx = tileVectorToPixelVector(cameraPosition);
        const auto boxInPixels = BoundingBox{
          tileVectorToPixelVector(link.mLinkedGeometrySection.topLeft) -
            worldToScreenPx,
          tileExtentsToPixelExtents(link.mLinkedGeometrySection.size)};

        mpRenderer->drawRectangle(boxInPixels, base::Color{0, 255, 255, 190});
      });
  }

  if (mShowGrid)
  {
    const auto drawColor = base::Color{255, 255, 255, 190};
    const auto maxX = tilesToPixels(viewportSize.width);
    const auto maxY = tilesToPixels(viewportSize.height);

    // Horizontal lines
    for (int y = 0; y < viewportSize.height; ++y)
    {
      const auto pxY = tilesToPixels(y);
      mpRenderer->drawLine(0, pxY, maxX, pxY, drawColor);
    }

    // Vertical lines
    for (int x = 0; x < viewportSize.width; ++x)
    {
      const auto pxX = tilesToPixels(x);
      mpRenderer->drawLine(pxX, 0, pxX, maxY, drawColor);
    }
  }
}

} // namespace rigel::game_logic
