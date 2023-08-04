/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/bonus.hpp"
#include "data/game_session_data.hpp"
#include "data/map.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/graphical_effects.hpp"
#include "engine/map_renderer.hpp"
#include "engine/sprite_factory.hpp"
#include "frontend/game_mode.hpp"
#include "game_logic_common/igame_world.hpp"
#include "game_logic_common/input.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"
#include "ui/menu_element_renderer.hpp"


struct Context_;

namespace rigel::assets
{
class ResourceLoader;
}

namespace rigel::game_logic
{

namespace detail
{

using State = Context_;


struct SpriteDrawCmd
{
  std::uint16_t id;
  std::uint16_t frame;
  std::uint16_t x;
  std::uint16_t y;
  std::uint16_t drawStyle;
};


struct PixelDrawCmd
{
  std::uint16_t x;
  std::uint16_t y;
  std::uint8_t color;
};


struct TileDrawCmd
{
  std::uint16_t tileIndex;
  std::uint16_t x;
  std::uint16_t y;
};


struct WaterAreaDrawCmd
{
  std::uint16_t left;
  std::uint16_t top;
  std::uint16_t animStep;
};


struct Bridge
{
  Bridge(
    const assets::ResourceLoader& resources,
    data::map::Map* pMap,
    IGameServiceProvider* pServiceProvider,
    ui::IngameMessageDisplay* pMessageDisplay,
    data::PersistentPlayerState* pPersistentPlayerState);

  void resetForNewFrame();

  std::vector<SpriteDrawCmd> mSpritesToDraw;
  std::vector<PixelDrawCmd> mPixelsToDraw;
  std::vector<TileDrawCmd> mTileDebrisToDraw;
  std::vector<WaterAreaDrawCmd> mWaterAreasToDraw;
  std::vector<base::Vec2> mRadarDots;
  std::uint8_t mScreenShift = 0;

  const char* mpErrorMessage = nullptr;

  data::LevelHints mLevelHints;

  data::map::Map* mpMap;
  engine::MapRenderer* mpMapRenderer = nullptr;
  IGameServiceProvider* mpServiceProvider;
  ui::IngameMessageDisplay* mpMessageDisplay;
  data::PersistentPlayerState* mpPersistentPlayerState;
};

} // namespace detail


class GameWorld_Classic : public IGameWorld
{
public:
  GameWorld_Classic(
    data::PersistentPlayerState* pPersistentPlayerState,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vec2> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false,
    const PlayerInput& initialInput = PlayerInput{});
  ~GameWorld_Classic() override;

  GameWorld_Classic(const GameWorld_Classic&) = delete;
  GameWorld_Classic& operator=(const GameWorld_Classic&) = delete;

  bool levelFinished() const override;
  std::set<data::Bonus> achievedBonuses() const override;

  bool needsPerElementUpscaling() const override;

  void updateGameLogic(const PlayerInput& input) override;
  void render(float interpolationFactor = 0.0f) override;
  void processEndOfFrameActions() override;
  void updateBackdropAutoScrolling(engine::TimeDelta dt) override;

  bool isPlayerInShip() const override;
  void toggleGodMode() override;
  bool isGodModeOn() const override;

  void activateFullHealthCheat() override;
  void activateGiveItemsCheat() override;

  void quickSave() override;
  void quickLoad() override;
  bool canQuickLoad() const override;

  void debugToggleBoundingBoxDisplay() override { }
  void debugToggleWorldCollisionDataDisplay() override { }
  void debugToggleGridDisplay() override { }
  void printDebugText(std::ostream& stream) const override;

private:
  void drawWorld();
  void drawMapAndSprites(const base::Rect<int>& region);
  void updateVisibleWaterAreas();
  void loadLevel(const data::GameSessionId& sessionId);
  void syncBackdrop();
  void syncPersistentPlayerState();

  struct QuickSaveData;

  renderer::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TiledTexture mUiSpriteSheet;
  ui::MenuElementRenderer mTextRenderer;
  data::PersistentPlayerState* mpPersistentPlayerState;
  const data::GameOptions* mpOptions;
  const assets::ResourceLoader* mpResources;
  engine::SpriteFactory* mpSpriteFactory;
  std::vector<int> mImageIdTable;

  data::GameSessionId mSessionId;
  std::string mMusicFile;
  bool mIsUsingSecondaryBackdrop = false;

  data::map::Map mMap;
  std::optional<engine::MapRenderer> mMapRenderer;
  data::PersistentPlayerState mPersistentPlayerStateAtLevelStart;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  engine::SpecialEffectsRenderer mSpecialEffects;
  renderer::RenderTargetTexture mLowResLayer;
  std::vector<engine::WaterEffectArea> mVisibleWaterAreas;
  base::Size mPreviousWindowSize;
  bool mPerElementUpscalingWasEnabled;

  detail::Bridge mBridge;
  std::unique_ptr<detail::State> mpState;
  std::unique_ptr<QuickSaveData> mpQuickSave;

  std::optional<data::PersistentPlayerState::CheckpointState> mCheckpointState;
};

} // namespace rigel::game_logic
