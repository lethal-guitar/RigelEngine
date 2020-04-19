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

#include "damage_system.hpp"

#include "common/global.hpp"
#include "data/player_model.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::player {

using engine::components::BoundingBox;
using engine::components::WorldPosition;
using engine::toWorldSpace;
using game_logic::components::PlayerDamaging;


DamageSystem::DamageSystem(Player* pPlayer, entityx::EventManager* pEvents)
  : mpPlayer(pPlayer)
  , mpEvents(pEvents)
{
}


void DamageSystem::update(entityx::EntityManager& es) {
  if (mpPlayer->isDead()) {
    return;
  }

  const auto playerBBox = mpPlayer->worldSpaceHitBox();
  es.each<PlayerDamaging, BoundingBox, WorldPosition>(
    [this, &playerBBox](
      entityx::Entity entity,
      const PlayerDamaging& damage,
      const BoundingBox& boundingBox,
      const WorldPosition& position
    ) {
      const auto bbox = toWorldSpace(boundingBox, position);
      const auto hasCollision = bbox.intersects(playerBBox);
      const auto playerCanTakeDamage =
        mpPlayer->canTakeDamage() || damage.mIsFatal;

      if (hasCollision && playerCanTakeDamage) {
        mpEvents->emit(rigel::events::PlayerTookDamage{});

        if (damage.mIsFatal) {
          mpPlayer->takeFatalDamage();
        } else {
          mpPlayer->takeDamage(damage.mAmount);
        }
        if (damage.mDestroyOnContact) {
          entity.destroy();
        }
      }
    });
}

}
