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
#include "game_logic/input.hpp"
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

// Update game logic at 15 FPS. This is not exactly the speed at which the
// game runs on period-appropriate hardware, but it's very close, and it nicely
// fits into 60 FPS, giving us 4 render frames for 1 logic update.
//
// On a 486 with a fast graphics card, the game runs at roughly 15.5 FPS, with
// a slower (non-VLB) graphics card, it's roughly 14 FPS. On a fast 386 (40
// MHz), it's roughly 13 FPS. With 15 FPS, the feel should therefore be very
// close to playing the game on a 486 at the default game speed setting.
constexpr auto GAME_LOGIC_UPDATE_DELAY = 1.0 / 15.0;


struct WorldState;

class GameWorld : public entityx::Receiver<GameWorld>
{
public:
  GameWorld(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vec2> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false,
    const PlayerInput& initialInput = PlayerInput{});
  ~GameWorld(); // NOLINT

  bool levelFinished() const;
  std::set<data::Bonus> achievedBonuses() const;

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

  bool needsPerElementUpscaling() const;
  void updateGameLogic(const PlayerInput& input);
  void render(float interpolationFactor = 0.0f);
  void processEndOfFrameActions();

  void activateFullHealthCheat();
  void activateGiveItemsCheat();

  void quickSave();
  void quickLoad();
  bool canQuickLoad() const;

  friend class rigel::GameRunner;

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

  void printDebugText(std::ostream& stream) const;

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
