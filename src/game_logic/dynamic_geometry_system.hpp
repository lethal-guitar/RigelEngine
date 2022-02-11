/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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
#include "engine/map_renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <vector>


namespace rigel
{
struct IGameServiceProvider;

namespace engine
{
class CollisionChecker;
class RandomNumberGenerator;
} // namespace engine
namespace events
{
struct DoorOpened;
struct MissileDetonated;
struct TileBurnedAway;
} // namespace events
namespace game_logic::events
{
struct ShootableKilled;
}
} // namespace rigel


namespace rigel::game_logic
{

struct FallingSectionInfo
{
  base::Rect<int> mSectionBelow;
  int mIndex;
};


struct DynamicMapSectionData
{
  data::map::Map mMapStaticParts;

  std::vector<base::Rect<int>> mSimpleSections;
  std::vector<FallingSectionInfo> mFallingSections;
};


DynamicMapSectionData determineDynamicMapSections(
  const data::map::Map& originalMap,
  const std::vector<data::map::LevelData::Actor>& actorDescriptions);


class DynamicGeometrySystem : public entityx::Receiver<DynamicGeometrySystem>
{
public:
  DynamicGeometrySystem(
    IGameServiceProvider* pServiceProvider,
    entityx::EntityManager* pEntityManager,
    data::map::Map* pMap,
    engine::RandomNumberGenerator* pRandomGenerator,
    entityx::EventManager* pEvents,
    engine::MapRenderer* pMapRenderer,
    std::vector<base::Rect<int>> simpleDynamicSections);

  void initializeDynamicGeometryEntities(
    const std::vector<FallingSectionInfo>& fallingSections);

  void receive(const events::ShootableKilled& event);
  void receive(const rigel::events::DoorOpened& event);
  void receive(const rigel::events::MissileDetonated& event);
  void receive(const rigel::events::TileBurnedAway& event);

  void renderDynamicBackgroundSections(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize,
    float interpolationFactor);
  void renderDynamicForegroundSections(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize,
    float interpolationFactor);

private:
  void renderDynamicSections(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize,
    float interpolationFactor,
    engine::MapRenderer::DrawMode drawMode);

  IGameServiceProvider* mpServiceProvider;
  entityx::EntityManager* mpEntityManager;
  data::map::Map* mpMap;
  engine::RandomNumberGenerator* mpRandomGenerator;
  entityx::EventManager* mpEvents;
  engine::MapRenderer* mpMapRenderer;
  std::vector<base::Rect<int>> mSimpleDynamicSections;
};

} // namespace rigel::game_logic
