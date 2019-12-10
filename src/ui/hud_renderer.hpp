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

#include "data/player_model.hpp"
#include "engine/tiled_texture.hpp"
#include "renderer/texture.hpp"

#include <cstdint>
#include <unordered_map>


namespace rigel {

namespace data {
  class Image;
  class PlayerModel;
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
    int levelNumber,
    renderer::Renderer* pRenderer,
    const loader::ResourceLoader& bundle,
    engine::TiledTexture* pStatusSpriteSheetRenderer);

  void updateAnimation();
  void render(const data::PlayerModel& playerModel);

private:
  struct CollectedLetterIndicator {
    renderer::OwningTexture mTexture;
    base::Vector mPxPosition;
  };

  using InventoryItemTextureMap =
    std::unordered_map<data::InventoryItemType, renderer::OwningTexture>;
  using CollectedLetterIndicatorMap =
    std::unordered_map<data::CollectableLetterType, CollectedLetterIndicator>;

  HudRenderer(
    int levelNumber,
    renderer::Renderer* pRenderer,
    const loader::ActorData& actorData,
    InventoryItemTextureMap&& inventoryItemTextures,
    CollectedLetterIndicatorMap&& collectedLetterTextures,
    engine::TiledTexture* pStatusSpriteSheetRenderer);

  static InventoryItemTextureMap makeInventoryItemTextureMap(
    renderer::Renderer* pRenderer,
    const loader::ActorImagePackage& imagePack);

  static CollectedLetterIndicatorMap makeCollectedLetterTextureMap(
    renderer::Renderer* pRenderer,
    const loader::ActorImagePackage& imagePack);

  void drawHealthBar(const data::PlayerModel& playerModel) const;
  void drawCollectedLetters(const data::PlayerModel& playerModel) const;
  void drawRadar() const;

  const int mLevelNumber;
  renderer::Renderer* mpRenderer;

  std::uint32_t mElapsedFrames = 0;

  renderer::OwningTexture mTopRightTexture;
  renderer::OwningTexture mBottomLeftTexture;
  renderer::OwningTexture mBottomRightTexture;
  InventoryItemTextureMap mInventoryTexturesByType;
  CollectedLetterIndicatorMap mCollectedLetterIndicatorsByType;
  engine::TiledTexture* mpStatusSpriteSheetRenderer;
  mutable renderer::RenderTargetTexture mRadarSurface;
};

}}
