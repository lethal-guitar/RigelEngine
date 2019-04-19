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

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/bonus.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/earth_quake_effect.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/enemy_radar.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/input.hpp"
#include "game_logic/player/components.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"

#include "game_mode.hpp"
#include "global_level_events.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <optional>
#include <vector>

namespace rigel { namespace game_logic { class IngameSystems; }}


namespace rigel {

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

  void receive(const events::CheckPointActivated& event);
  void receive(const events::ExitReached& event);
  void receive(const events::PlayerDied& event);
  void receive(const events::PlayerTookDamage& event);
  void receive(const events::PlayerMessage& event);
  void receive(const events::PlayerTeleported& event);
  void receive(const events::ScreenFlash& event);
  void receive(const rigel::events::ScreenShake& event);
  void receive(const events::TutorialMessage& event);
  void receive(const game_logic::events::ShootableKilled& event);
  void receive(const rigel::events::BossActivated& event);

  void updateGameLogic(const game_logic::PlayerInput& input);
  void render();
  void processEndOfFrameActions();

  friend class GameRunner;

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

  engine::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TileRenderer* mpUiSpriteSheet;
  ui::MenuElementRenderer* mpTextRenderer;
  entityx::EventManager mEventManager;
  entityx::EntityManager mEntities;
  game_logic::EntityFactory mEntityFactory;

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

  std::unique_ptr<game_logic::IngameSystems> mpSystems;

  game_logic::RadarDishCounter mRadarDishCounter;
  engine::RandomNumberGenerator mRandomGenerator;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;

  std::optional<engine::EarthQuakeEffect> mEarthQuakeEffect;
  std::optional<base::Color> mScreenFlashColor;
  std::optional<base::Color> mBackdropFlashColor;
  std::optional<int> mReactorDestructionFramesElapsed;
  int mScreenShakeOffsetX = 0;
};


class GameRunner : public entityx::Receiver<GameRunner> {
public:
  GameRunner(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);

  bool levelFinished() const;
  std::set<data::Bonus> achievedBonuses() const;

private:
  GameWorld mWorld;
  game_logic::PlayerInput mPlayerInput;
  engine::TimeDelta mAccumulatedTime = 0.0;
  bool mShowDebugText = false;
  bool mSingleStepping = false;
  bool mDoNextSingleStep = false;

};


inline bool GameRunner::levelFinished() const {
  return mWorld.levelFinished();
}


inline std::set<data::Bonus> GameRunner::achievedBonuses() const {
  return mWorld.achievedBonuses();
}

}
