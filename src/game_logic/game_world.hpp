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
#include "common/game_mode.hpp"
#include "common/global.hpp"
#include "data/bonus.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/earth_quake_effect.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/input.hpp"
#include "game_logic/interactive/enemy_radar.hpp"
#include "game_logic/player/components.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <optional>
#include <vector>

namespace rigel { class GameRunner; }
namespace rigel::data { struct GameOptions; }


namespace rigel::game_logic {

class IngameSystems;


class GameWorld : public entityx::Receiver<GameWorld> {
public:
  GameWorld(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);
  ~GameWorld(); // NOLINT

  bool levelFinished() const;
  std::set<data::Bonus> achievedBonuses() const;

  void receive(const rigel::events::CheckPointActivated& event);
  void receive(const rigel::events::ExitReached& event);
  void receive(const rigel::events::PlayerDied& event);
  void receive(const rigel::events::PlayerTookDamage& event);
  void receive(const rigel::events::PlayerMessage& event);
  void receive(const rigel::events::PlayerTeleported& event);
  void receive(const rigel::events::ScreenFlash& event);
  void receive(const rigel::events::ScreenShake& event);
  void receive(const rigel::events::TutorialMessage& event);
  void receive(const events::ShootableKilled& event);
  void receive(const rigel::events::BossActivated& event);

  void updateGameLogic(const PlayerInput& input);
  void render(float interpolation);
  void processEndOfFrameActions();

  friend class rigel::GameRunner;

private:
  void loadLevel(
    const data::GameSessionId& sessionId,
    const loader::ResourceLoader& resources);

  void onReactorDestroyed(const base::Vector& position);
  void updateReactorDestructionEvent();

  void handleLevelExit();
  void handlePlayerDeath();
  void restartLevel();
  void restartFromCheckpoint();
  void handleTeleporter();
  void updateTemporaryItemExpiration();
  void showTutorialMessage(const data::TutorialMessageId id);

  void showDebugText();

private:
  struct LevelBonusInfo {
    int mInitialCameraCount = 0;
    int mInitialMerchandiseCount = 0;
    int mInitialWeaponCount = 0;
    int mInitialLaserTurretCount = 0;
    int mInitialBonusGlobeCount = 0;

    int mNumShotBonusGlobes = 0;
    bool mPlayerTookDamage = false;
  };

  struct CheckpointData {
    data::PlayerModel::CheckpointState mState;
    base::Vector mPosition;
  };

  renderer::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TiledTexture* mpUiSpriteSheet;
  ui::MenuElementRenderer* mpTextRenderer;
  entityx::EventManager mEventManager;
  entityx::EntityManager mEntities;
  EntityFactory mEntityFactory;

  data::PlayerModel* mpPlayerModel;
  data::PlayerModel mPlayerModelAtLevelStart;
  LevelBonusInfo mBonusInfo;
  std::optional<CheckpointData> mActivatedCheckpoint;
  std::optional<std::string> mLevelMusicFile;

  std::optional<base::Vector> mTeleportTargetPosition;
  entityx::Entity mActiveBossEntity;
  bool mBackdropSwitched = false;
  bool mLevelFinished = false;
  bool mPlayerDied = false;

  struct LevelData {
    data::map::Map mMap;
    std::vector<data::map::LevelData::Actor> mInitialActors;
    data::map::BackdropSwitchCondition mBackdropSwitchCondition;
  };

  LevelData mLevelData;
  data::map::Map mMapAtLevelStart;

  std::unique_ptr<IngameSystems> mpSystems;

  RadarDishCounter mRadarDishCounter;
  engine::RandomNumberGenerator mRandomGenerator;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  const data::GameOptions* mpOptions;

  std::optional<EarthQuakeEffect> mEarthQuakeEffect;
  std::optional<base::Color> mScreenFlashColor;
  std::optional<base::Color> mBackdropFlashColor;
  std::optional<int> mReactorDestructionFramesElapsed;
  int mScreenShakeOffsetX = 0;
};

}
