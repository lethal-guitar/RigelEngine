/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "data/map.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel::game_logic {

using ProjectileType = game_logic::components::PlayerProjectile::Type;


enum class ProjectileDirection {
  Left,
  Right,
  Up,
  Down
};


enum class SpriteMovement {
  FlyRight = 0,
  FlyUpperRight = 1,
  FlyUp = 2,
  FlyUpperLeft = 3,
  FlyLeft = 4,
  FlyDown = 5,
  SwirlAround = 6
};


enum class ScoreNumberType : std::uint8_t {
  S100,
  S500,
  S2000,
  S5000,
  S10000
};


constexpr ScoreNumberType ScoreNumberType_Items[] = {
  ScoreNumberType::S10000,
  ScoreNumberType::S5000,
  ScoreNumberType::S2000,
  ScoreNumberType::S500,
  ScoreNumberType::S100
};

constexpr int ScoreNumberType_Values[] = {100, 500, 2000, 5000, 10000};


constexpr int scoreNumberValue(const ScoreNumberType type) {
  return ScoreNumberType_Values[static_cast<std::size_t>(type)];
}


struct IEntityFactory {
  virtual ~IEntityFactory() = default;

  virtual void createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors) = 0;

  virtual engine::components::Sprite createSpriteForId(
    const data::ActorID actorID) = 0;

  /** Create a sprite entity using the given actor ID. If assignBoundingBox is
   * true, the dimensions of the sprite's first frame are used to assign a
   * bounding box.
   */
  virtual entityx::Entity spawnSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false) = 0;

  virtual entityx::Entity spawnSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false) = 0;

  virtual entityx::Entity spawnProjectile(
    ProjectileType type,
    const engine::components::WorldPosition& pos,
    ProjectileDirection direction) = 0;

  virtual entityx::Entity spawnActor(
    data::ActorID actorID,
    const base::Vector& position) = 0;

  virtual entityx::EntityManager& entityManager() = 0;
};


/** Creates a temporary sprite (destroyed after showing last animation frame)
 *
 * This sets up a sprite entity using the sprite corresponding to the given
 * actor ID, which is set up to play all animation frames in the sprite and
 * then disappear.
 */
entityx::Entity spawnOneShotSprite(
  IEntityFactory& factory,
  data::ActorID id,
  const base::Vector& position);


entityx::Entity spawnFloatingOneShotSprite(
  IEntityFactory& factory,
  data::ActorID id,
  const base::Vector& position);


entityx::Entity spawnMovingEffectSprite(
  IEntityFactory& factory,
  const data::ActorID id,
  const SpriteMovement movement,
  const base::Vector& position
);


void spawnFloatingScoreNumber(
  IEntityFactory& factory,
  ScoreNumberType type,
  const base::Vector& position);


void spawnFireEffect(
  entityx::EntityManager& entityManager,
  const base::Vector& position,
  const engine::components::BoundingBox& coveredArea,
  data::ActorID actorToSpawn);


void spawnEnemyLaserShot(
  IEntityFactory& factory,
  base::Vector position,
  engine::components::Orientation orientation);

}
