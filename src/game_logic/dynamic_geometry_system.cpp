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

#include "dynamic_geometry_system.hpp"

#include "common/game_service_provider.hpp"
#include "data/map.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/motion_smoothing.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic
{

using game_logic::components::MapGeometryLink;

namespace
{

constexpr auto GEOMETRY_FALL_SPEED = 2;

const base::Vec2f TILE_DEBRIS_MOVEMENT_SEQUENCE[] = {
  {0.0f, -3.0f},
  {0.0f, -3.0f},
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f, 0.0f},
  {0.0f, 0.0f},
  {0.0f, 1.0f},
  {0.0f, 2.0f},
  {0.0f, 2.0f},
  {0.0f, 3.0f}};


void spawnTileDebris(
  entityx::EntityManager& entities,
  const int x,
  const int y,
  const data::map::TileIndex tileIndex,
  const int velocityX,
  const int ySequenceOffset)
{
  using namespace engine::components;
  using namespace engine::components::parameter_aliases;
  using components::TileDebris;

  auto movement = MovementSequence{
    TILE_DEBRIS_MOVEMENT_SEQUENCE, ResetAfterSequence{false}, EnableX{false}};
  movement.mCurrentStep = ySequenceOffset;

  auto debris = entities.create();
  debris.assign<WorldPosition>(x, y);
  debris.assign<BoundingBox>(BoundingBox{{}, {1, 1}});
  debris.assign<Active>();
  debris.assign<ActivationSettings>(ActivationSettings::Policy::Always);
  debris.assign<AutoDestroy>(AutoDestroy::afterTimeout(80));
  debris.assign<TileDebris>(TileDebris{tileIndex});
  debris.assign<MovingBody>(
    Velocity{static_cast<float>(velocityX), 0.0f},
    GravityAffected{false},
    IgnoreCollisions{true});
  debris.assign<MovementSequence>(movement);
  engine::enableInterpolation(debris);
}


void spawnTileDebrisForSection(
  const engine::components::BoundingBox& mapSection,
  data::map::Map& map,
  entityx::EntityManager& entities,
  engine::RandomNumberGenerator& randomGen)
{
  const auto& start = mapSection.topLeft;
  const auto& size = mapSection.size;
  for (auto y = start.y; y < start.y + size.height; ++y)
  {
    for (auto x = start.x; x < start.x + size.width; ++x)
    {
      const auto tileIndex = map.tileAt(0, x, y);
      if (tileIndex == 0)
      {
        continue;
      }

      const auto velocityX = 3 - randomGen.gen() % 6;
      const auto ySequenceOffset = randomGen.gen() % 5;
      spawnTileDebris(entities, x, y, tileIndex, velocityX, ySequenceOffset);
    }
  }
}


void explodeMapSection(
  const base::Rect<int>& mapSection,
  data::map::Map& map,
  entityx::EntityManager& entityManager,
  engine::RandomNumberGenerator& randomGenerator)
{
  spawnTileDebrisForSection(mapSection, map, entityManager, randomGenerator);

  map.clearSection(
    mapSection.topLeft.x,
    mapSection.topLeft.y,
    mapSection.size.width,
    mapSection.size.height);
}


void explodeMapSection(
  const base::Rect<int>& mapSection,
  GlobalDependencies& d,
  GlobalState& s)
{
  explodeMapSection(
    mapSection, *s.mpMap, *d.mpEntityManager, *d.mpRandomGenerator);
}


void moveTileRows(const base::Rect<int>& mapSection, data::map::Map& map)
{
  const auto startX = mapSection.left();
  const auto startY = mapSection.top();
  const auto width = mapSection.size.width;
  const auto height = mapSection.size.height;

  for (int layer = 0; layer < 2; ++layer)
  {
    for (int row = 0; row < height; ++row)
    {
      for (int col = 0; col < width; ++col)
      {
        const auto x = startX + col;
        const auto sourceY = startY + (height - row - 1);
        const auto targetY = sourceY + 1;

        map.setTileAt(layer, x, targetY, map.tileAt(layer, x, sourceY));
      }
    }
  }

  map.clearSection(startX, startY, width, 1);
}


void moveTileSection(base::Rect<int>& mapSection, data::map::Map& map)
{
  moveTileRows(mapSection, map);
  ++mapSection.topLeft.y;
}


void squashTileSection(base::Rect<int>& mapSection, data::map::Map& map)
{
  // By not moving the lower-most row, it gets effectively overwritten by the
  // row above
  moveTileRows(
    {mapSection.topLeft, {mapSection.size.width, mapSection.size.height - 1}},
    map);
  ++mapSection.topLeft.y;
  --mapSection.size.height;
}

} // namespace


DynamicGeometrySystem::DynamicGeometrySystem(
  IGameServiceProvider* pServiceProvider,
  entityx::EntityManager* pEntityManager,
  data::map::Map* pMap,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EventManager* pEvents)
  : mpServiceProvider(pServiceProvider)
  , mpEntityManager(pEntityManager)
  , mpMap(pMap)
  , mpRandomGenerator(pRandomGenerator)
  , mpEvents(pEvents)
{
  pEvents->subscribe<events::ShootableKilled>(*this);
  pEvents->subscribe<rigel::events::DoorOpened>(*this);
  pEvents->subscribe<rigel::events::MissileDetonated>(*this);
}


void DynamicGeometrySystem::receive(const events::ShootableKilled& event)
{
  auto entity = event.mEntity;
  // Take care of shootable walls
  if (!entity.has_component<MapGeometryLink>())
  {
    return;
  }

  const auto& mapSection =
    entity.component<MapGeometryLink>()->mLinkedGeometrySection;
  explodeMapSection(mapSection, *mpMap, *mpEntityManager, *mpRandomGenerator);
  mpServiceProvider->playSound(data::SoundId::BigExplosion);
  mpEvents->emit(rigel::events::ScreenFlash{});
}


void DynamicGeometrySystem::receive(const rigel::events::DoorOpened& event)
{
  using namespace engine::components;
  using namespace game_logic::components;

  auto entity = event.mEntity;
  entity.remove<ActorTag>();
  entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
  entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
    behaviors::DynamicGeometryController::Type::BlueKeyDoor});
}


void DynamicGeometrySystem::receive(
  const rigel::events::MissileDetonated& event)
{
  // TODO: Add a helper function for creating rectangles based on different
  // given values, e.g. bottom left and size
  engine::components::BoundingBox mapSection{
    event.mImpactPosition - base::Vec2{0, 2}, {3, 3}};
  explodeMapSection(mapSection, *mpMap, *mpEntityManager, *mpRandomGenerator);
  mpEvents->emit(rigel::events::ScreenFlash{});
}


void behaviors::DynamicGeometryController::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& position = *entity.component<WorldPosition>();
  auto& mapSection =
    entity.component<MapGeometryLink>()->mLinkedGeometrySection;

  auto isOnSolidGround = [&]() {
    if (mapSection.bottom() >= s.mpMap->height() - 1)
    {
      return true;
    }

    const auto bottomLeft =
      s.mpMap->collisionData(mapSection.left(), mapSection.bottom() + 1);
    const auto bottomRight =
      s.mpMap->collisionData(mapSection.right(), mapSection.bottom() + 1);
    return bottomLeft.isSolidOn(data::map::SolidEdge::top()) ||
      bottomRight.isSolidOn(data::map::SolidEdge::top());
  };

  auto makeAlwaysActive = [&]() {
    engine::reassign<ActivationSettings>(
      entity, ActivationSettings::Policy::Always);
  };

  auto updateWaiting = [&](const int numFrames) {
    ++mFramesElapsed;
    if (mFramesElapsed == numFrames)
    {
      d.mpServiceProvider->playSound(data::SoundId::FallingRock);
    }
    else if (mFramesElapsed == numFrames + 1)
    {
      makeAlwaysActive();
      mState = State::Falling;
    }
  };

  auto fall = [&]() {
    for (int i = 0; i < GEOMETRY_FALL_SPEED; ++i)
    {
      if (isOnSolidGround())
      {
        return true;
      }

      moveTileSection(mapSection, *s.mpMap);
      ++position.y;
    }

    return false;
  };

  auto doBurnEffect = [&]() {
    d.mpEvents->emit(rigel::events::ScreenShake{2});
    d.mpServiceProvider->playSound(data::SoundId::HammerSmash);

    const auto offset = d.mpRandomGenerator->gen() % mapSection.size.width;
    const auto spawnPosition =
      base::Vec2{mapSection.left() + offset, mapSection.bottom() + 1};
    spawnFloatingOneShotSprite(
      *d.mpEntityFactory, data::ActorID::Shot_impact_FX, spawnPosition);
  };

  auto sink = [&]() {
    if (mapSection.size.height == 1)
    {
      s.mpMap->clearSection(
        mapSection.topLeft.x, mapSection.topLeft.y, mapSection.size.width, 1);
      d.mpServiceProvider->playSound(data::SoundId::BlueKeyDoorOpened);
      entity.destroy();
    }
    else
    {
      squashTileSection(mapSection, *s.mpMap);
      ++position.y;
    }
  };

  auto land = [&]() {
    d.mpServiceProvider->playSound(data::SoundId::BlueKeyDoorOpened);
    d.mpEvents->emit(rigel::events::ScreenShake{7});
  };

  auto explode = [&]() {
    explodeMapSection(mapSection, d, s);
    d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
    entity.destroy();
  };


  auto updateType1 = [&, this]() {
    switch (mState)
    {
      case State::Waiting:
        updateWaiting(20);
        break;

      case State::Falling:
        if (fall())
        {
          mState = State::Sinking;
          doBurnEffect();
          sink();
        }
        break;

      case State::Sinking:
        doBurnEffect();
        sink();
        break;

      default:
        break;
    }
  };


  auto updateType2 = [&, this]() {
    switch (mState)
    {
      case State::Waiting:
        if (s.mpPerFrameState->mIsEarthShaking)
        {
          updateWaiting(2);
        }
        break;

      case State::Falling:
        if (fall())
        {
          explode();
        }
        break;

      default:
        break;
    }
  };


  auto updateType3 = [&, this]() {
    switch (mState)
    {
      case State::Waiting:
        if (!isOnSolidGround())
        {
          makeAlwaysActive();
          mState = State::Falling;
          if (fall())
          {
            land();
            mState = State::Waiting;
          }
        }
        break;

      case State::Falling:
        if (fall())
        {
          land();
          mState = State::Waiting;
        }
        break;

      default:
        break;
    }
  };


  auto updateType5 = [&, this]() {
    switch (mState)
    {
      case State::Waiting:
        if (!isOnSolidGround())
        {
          makeAlwaysActive();
          mState = State::Falling;
          if (fall())
          {
            explode();
          }
        }
        break;

      case State::Falling:
        if (fall())
        {
          explode();
        }
        break;

      default:
        break;
    }
  };


  auto updateType6 = [&, this]() {
    switch (mState)
    {
      case State::Waiting:
        updateWaiting(20);
        break;

      case State::Falling:
        for (int i = 0; i < GEOMETRY_FALL_SPEED; ++i)
        {
          if (isOnSolidGround())
          {
            land();
            mType = Type::FallDownImmediatelyThenStayOnGround;
            mState = State::Waiting;
          }
          else
          {
            moveTileSection(mapSection, *s.mpMap);
            ++position.y;
          }
        }
        break;

      default:
        break;
    }
  };


  auto updateDoor = [&]() {
    switch (mState)
    {
      case State::Waiting:
        ++mFramesElapsed;
        if (mFramesElapsed == 2)
        {
          makeAlwaysActive();
          mState = State::Falling;
        }
        break;

      case State::Falling:
        if (fall())
        {
          mState = State::Sinking;
          sink();
        }
        break;

      case State::Sinking:
        sink();
        break;

      default:
        break;
    }
  };


  switch (mType)
  {
    case Type::FallDownAfterDelayThenSinkIntoGround:
      updateType1();
      break;

    case Type::BlueKeyDoor:
      updateDoor();
      break;

    case Type::FallDownWhileEarthQuakeActiveThenExplode:
      updateType2();
      break;

    case Type::FallDownImmediatelyThenStayOnGround:
      updateType3();
      break;

    case Type::FallDownImmediatelyThenExplode:
      updateType5();
      break;

    case Type::FallDownAfterDelayThenStayOnGround:
      updateType6();
      break;

    default:
      break;
  }
}

} // namespace rigel::game_logic
