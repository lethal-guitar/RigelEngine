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

#include "base/warnings.hpp"
#include "data/game_session_data.hpp"
#include "engine/base_components.hpp"
#include "engine/renderer.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/ientity_factory.hpp"
#include "loader/level_loader.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <unordered_map>
#include <vector>


namespace rigel { namespace loader { class ActorImagePackage; }}

namespace rigel { namespace game_logic {

enum class ContainerColor {
  Red,
  Green,
  White,
  Blue
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


inline bool isHorizontal(const ProjectileDirection direction) {
  return
    direction == ProjectileDirection::Left ||
    direction == ProjectileDirection::Right;
}


class SpriteFactory {
public:
  SpriteFactory(
    engine::Renderer* pRenderer,
    const loader::ActorImagePackage* pSpritePackage);

  engine::components::Sprite createSprite(data::ActorID id);
  base::Rect<int> actorFrameRect(data::ActorID id, int frame) const;

private:
  struct SpriteData {
    engine::SpriteDrawData mDrawData;
    std::vector<int> mInitialFramesToRender;
  };

  engine::Renderer* mpRenderer;
  const loader::ActorImagePackage* mpSpritePackage;
  std::unordered_map<data::ActorID, SpriteData> mSpriteDataCache;
};


class EntityFactory : public IEntityFactory {
public:
  EntityFactory(
    engine::Renderer* pRenderer,
    entityx::EntityManager* pEntityManager,
    const loader::ActorImagePackage* pSpritePackage,
    data::Difficulty difficulty);

  entityx::Entity createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors) override;

  /** Create a sprite entity using the given actor ID. If assignBoundingBox is
   * true, the dimensions of the sprite's first frame are used to assign a
   * bounding box.
   */
  entityx::Entity createSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false) override;

  entityx::Entity createSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false) override;

  entityx::Entity createProjectile(
    ProjectileType type,
    const engine::components::WorldPosition& pos,
    ProjectileDirection direction) override;

  entityx::Entity createActor(
    data::ActorID actorID,
    const base::Vector& position) override;

private:
  engine::components::Sprite createSpriteForId(const data::ActorID actorID);

  void configureEntity(
    entityx::Entity entity,
    const data::ActorID actorID,
    const engine::components::BoundingBox& boundingBox);

  template<typename... Args>
  void configureItemBox(
    entityx::Entity entity,
    ContainerColor color,
    int givenScore,
    Args&&... components);

  engine::components::Sprite createSpriteComponent(data::ActorID mainId);

  void configureProjectile(
    entityx::Entity entity,
    const ProjectileType type,
    engine::components::WorldPosition position,
    const ProjectileDirection direction,
    const engine::components::BoundingBox& boundingBox
  );

  SpriteFactory mSpriteFactory;
  entityx::EntityManager* mpEntityManager;
  int mSpawnIndex = 0;
  data::Difficulty mDifficulty;
};


/** Creates a temporary sprite (destroyed after showing last animation frame)
 *
 * This sets up a sprite entity using the sprite corresponding to the given
 * actor ID, which is set up to play all animation frames in the sprite and
 * then disappear.
 */
entityx::Entity createOneShotSprite(
  IEntityFactory& factory,
  data::ActorID id,
  const base::Vector& position);


entityx::Entity createFloatingOneShotSprite(
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

}}
