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
#include "engine/isprite_factory.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/ientity_factory.hpp"
#include "loader/level_loader.hpp"
#include "renderer/renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <optional>
#include <unordered_map>
#include <vector>


namespace rigel::engine { class RandomNumberGenerator; }
namespace rigel::loader { class ActorImagePackage; }

namespace rigel::game_logic {

enum class ContainerColor {
  Red,
  Green,
  White,
  Blue
};


inline bool isHorizontal(const ProjectileDirection direction) {
  return
    direction == ProjectileDirection::Left ||
    direction == ProjectileDirection::Right;
}


class EntityFactory : public IEntityFactory {
public:
  EntityFactory(
    engine::ISpriteFactory* pSpriteFactory,
    entityx::EntityManager* pEntityManager,
    engine::RandomNumberGenerator* pRandomGenerator,
    data::Difficulty difficulty);

  void createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors) override;

  engine::components::Sprite createSpriteForId(
    const data::ActorID actorID) override;

  /** Create a sprite entity using the given actor ID. If assignBoundingBox is
   * true, the dimensions of the sprite's first frame are used to assign a
   * bounding box.
   */
  entityx::Entity spawnSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false) override;

  entityx::Entity spawnSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false) override;

  entityx::Entity spawnProjectile(
    ProjectileType type,
    const engine::components::WorldPosition& pos,
    ProjectileDirection direction) override;

  entityx::Entity spawnActor(
    data::ActorID actorID,
    const base::Vector& position) override;

  entityx::EntityManager& entityManager() {
    return *mpEntityManager;
  }

private:
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

  engine::ISpriteFactory* mpSpriteFactory;
  entityx::EntityManager* mpEntityManager;
  engine::RandomNumberGenerator* mpRandomGenerator;
  int mSpawnIndex = 0;
  data::Difficulty mDifficulty;
};

}
