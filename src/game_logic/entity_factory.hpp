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
#include "loader/level_loader.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <map>
#include <tuple>
#include <vector>


namespace rigel { namespace loader { class ActorImagePackage; }}

namespace rigel { namespace game_logic {

enum class ProjectileType {
  PlayerRegularShot,
  PlayerLaserShot,
  PlayerRocketShot,
  PlayerFlameShot,
  EnemyLaserShot
};


enum class ProjectileDirection {
  Left,
  Right,
  Up,
  Down
};


inline bool isHorizontal(const ProjectileDirection direction) {
  return
    direction == ProjectileDirection::Left ||
    direction == ProjectileDirection::Right;
}


class EntityFactory {
public:
  EntityFactory(
    engine::Renderer* pRenderer,
    entityx::EntityManager* pEntityManager,
    const loader::ActorImagePackage* pSpritePackage,
    data::Difficulty difficulty);

  entityx::Entity createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors);

  /** Create a sprite entity using the given actor ID. If assignBoundingBox is
   * true, the dimensions of the sprite's first frame are used to assign a
   * bounding box.
   */
  entityx::Entity createSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false);

  entityx::Entity createSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false);

  entityx::Entity createProjectile(
    ProjectileType type,
    const engine::components::WorldPosition& pos,
    ProjectileDirection direction);

private:
  using IdAndFrameNr = std::pair<data::ActorID, std::size_t>;

  engine::components::Sprite createSpriteForId(const data::ActorID actorID);

  const engine::OwningTexture& getOrCreateTexture(
    const IdAndFrameNr& textureId);
  engine::components::Sprite makeSpriteFromActorIDs(
    const std::vector<data::ActorID>& actorIDs);

  void configureProjectile(
    entityx::Entity entity,
    const ProjectileType type,
    engine::components::WorldPosition position,
    const ProjectileDirection direction,
    const engine::components::BoundingBox& boundingBox
  );

  engine::Renderer* mpRenderer;
  entityx::EntityManager* mpEntityManager;
  const loader::ActorImagePackage* mpSpritePackage;
  data::Difficulty mDifficulty;

  std::map<IdAndFrameNr, engine::OwningTexture> mTextureCache;
};

}}
