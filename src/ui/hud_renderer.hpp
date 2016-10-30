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

#include <data/player_data.hpp>
#include <engine/tile_renderer.hpp>
#include <engine/timing.hpp>
#include <sdl_utils/texture.hpp>
#include <utils/enum_hash.hpp>

#include <unordered_map>


namespace rigel {

namespace data {
  class Image;
}

namespace loader {
  struct ActorData;
  class ActorImagePackage;
  class ResourceLoader;
}


namespace ui {

class HudRenderer {
public:
  HudRenderer(
    data::PlayerModel* pPlayerModel,
    int levelNumber,
    SDL_Renderer* pRenderer,
    const loader::ResourceLoader& bundle);

  void render();
  void update(engine::TimeDelta dt);

private:
  struct CollectedLetterIndicator {
    sdl_utils::OwningTexture mTexture;
    base::Vector mPxPosition;
  };

  using InventoryItemTextureMap =
    std::unordered_map<data::InventoryItemType, sdl_utils::OwningTexture>;
  using CollectedLetterIndicatorMap =
    std::unordered_map<data::CollectableLetterType, CollectedLetterIndicator>;

  HudRenderer(
    data::PlayerModel* pPlayerModel,
    int levelNumber,
    SDL_Renderer* pRenderer,
    const loader::ActorData& actorData,
    InventoryItemTextureMap&& inventoryItemTextures,
    CollectedLetterIndicatorMap&& collectedLetterTextures,
    const data::Image& statusSpriteSheetImage);

  static InventoryItemTextureMap makeInventoryItemTextureMap(
    SDL_Renderer* pRenderer,
    const loader::ActorImagePackage& imagePack);

  static CollectedLetterIndicatorMap makeCollectedLetterTextureMap(
    SDL_Renderer* pRenderer,
    const loader::ActorImagePackage& imagePack);

  void drawHealthBar() const;
  void drawCollectedLetters() const;

  data::PlayerModel* mpPlayerModel;
  const int mLevelNumber;
  SDL_Renderer* mpRenderer;
  engine::TimeStepper mTimeStepper;

  sdl_utils::OwningTexture mTopRightTexture;
  sdl_utils::OwningTexture mBottomLeftTexture;
  sdl_utils::OwningTexture mBottomRightTexture;
  InventoryItemTextureMap mInventoryTexturesByType;
  CollectedLetterIndicatorMap mCollectedLetterIndicatorsByType;
  engine::TileRenderer mStatusSpriteSheetRenderer;
};

}}
