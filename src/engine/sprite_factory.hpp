/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "data/game_traits.hpp"
#include "engine/isprite_factory.hpp"

#include <unordered_map>
#include <vector>


namespace rigel::loader { class ActorImagePackage; }
namespace rigel::renderer { class Renderer; }


namespace rigel::engine {

// The game draws player projectiles after drawing all regular actors, which
// makes them appear on top of everything. But in our case, they are rendered
// using the same mechanism as the other sprites, so we have to explicitly
// assign an order (which is higher than all regular actors' draw order).
constexpr auto PLAYER_PROJECTILE_DRAW_ORDER = data::GameTraits::maxDrawOrder + 1;
constexpr auto MUZZLE_FLASH_DRAW_ORDER = PLAYER_PROJECTILE_DRAW_ORDER + 1;
constexpr auto EFFECT_DRAW_ORDER = MUZZLE_FLASH_DRAW_ORDER + 1;


bool hasAssociatedSprite(data::ActorID actorID);


class SpriteFactory : public ISpriteFactory {
public:
  SpriteFactory(
    renderer::Renderer* pRenderer,
    const loader::ActorImagePackage* pSpritePackage);

  engine::components::Sprite createSprite(data::ActorID id) override;
  base::Rect<int> actorFrameRect(data::ActorID id, int frame) const override;

private:
  struct SpriteData {
    engine::SpriteDrawData mDrawData;
    std::vector<int> mInitialFramesToRender;
  };

  const SpriteData& createOrFindData(data::ActorID id) const;

  renderer::Renderer* mpRenderer;
  const loader::ActorImagePackage* mpSpritePackage;
  mutable std::unordered_map<data::ActorID, SpriteData> mSpriteDataCache;
};

}
