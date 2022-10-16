/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/graphical_effects.hpp"
#include "engine/sprite_factory.hpp"
#include "frontend/game_mode.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic_common/igame_world.hpp"
#include "game_logic_common/input.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"
#include "ui/menu_element_renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <iosfwd>
#include <optional>
#include <vector>


namespace rigel
{
class GameRunner;
}
namespace rigel::data
{
struct GameOptions;
}
namespace rigel::data::map
{
struct LevelData;
}


namespace rigel::game_logic
{

struct WorldState;

class GameWorld : public IGameWorld, public entityx::Receiver<GameWorld>
{
public:
  GameWorld(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vec2> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false,
    const PlayerInput& initialInput = PlayerInput{});
  ~GameWorld() override;

  bool levelFinished() const override;
  std::set<data::Bonus> achievedBonuses() const override;

  void receive(const rigel::events::CheckPointActivated& event);
  void receive(const rigel::events::ExitReached& event);
  void receive(const rigel::events::HintMachineMessage& event);
  void receive(const rigel::events::PlayerDied& event);
  void receive(const rigel::events::PlayerTookDamage& event);
  void receive(const rigel::events::PlayerMessage& event);
  void receive(const rigel::events::PlayerTeleported& event);
  void receive(const rigel::events::ScreenFlash& event);
  void receive(const rigel::events::ScreenShake& event);
  void receive(const rigel::events::TutorialMessage& event);
  void receive(const events::ShootableKilled& event);
  void receive(const rigel::events::BossActivated& event);
  void receive(const rigel::events::BossDestroyed& event);
  void receive(const rigel::events::CloakPickedUp& event);
  void receive(const rigel::events::CloakExpired& event);

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

  void debugToggleBoundingBoxDisplay() override;
  void debugToggleWorldCollisionDataDisplay() override;
  void debugToggleGridDisplay() override;
  void printDebugText(std::ostream& stream) const override;

private:
  struct ViewportParams
  {
    base::Vec2f mInterpolatedCameraPosition;
    base::Vec2 mCameraOffset;

    base::Vec2 mRenderStartPosition;
    base::Size mViewportSize;
  };

  void loadLevel(const PlayerInput& initialInput);
  void createNewState();
  void subscribe(entityx::EventManager& eventManager);
  void unsubscribe(entityx::EventManager& eventManager);

  void onReactorDestroyed(const base::Vec2& position);
  void updateReactorDestructionEvent();

  void handleLevelExit();
  void handlePlayerDeath();
  void restartLevel();
  void restartFromCheckpoint();
  void handleTeleporter();
  void updateTemporaryItemExpiration();
  void showTutorialMessage(const data::TutorialMessageId id);
  void flashScreen(const base::Color& color);

  ViewportParams determineSmoothScrollViewport(
    const base::Size& viewportSizeOriginal,
    const float interpolationFactor) const;
  void updateMotionSmoothingStates();

  void
    drawMapAndSprites(const ViewportParams& params, float interpolationFactor);
  bool widescreenModeOn() const;

  struct QuickSaveData
  {
    data::PlayerModel mPlayerModel;
    std::unique_ptr<WorldState> mpState;
  };

  renderer::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TiledTexture mUiSpriteSheet;
  ui::MenuElementRenderer mTextRenderer;
  data::PlayerModel* mpPlayerModel;
  const data::GameOptions* mpOptions;
  const assets::ResourceLoader* mpResources;
  engine::SpriteFactory* mpSpriteFactory;
  data::GameSessionId mSessionId;

  data::PlayerModel mPlayerModelAtLevelStart;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  engine::SpecialEffectsRenderer mSpecialEffects;
  renderer::RenderTargetTexture mLowResLayer;
  base::Size mPreviousWindowSize;
  data::WidescreenHudStyle mPreviousHudStyle;
  bool mWidescreenModeWasOn;
  bool mPerElementUpscalingWasEnabled;
  bool mMotionSmoothingWasEnabled;

  std::unique_ptr<WorldState> mpState;
  std::unique_ptr<QuickSaveData> mpQuickSave;
};

} // namespace rigel::game_logic
