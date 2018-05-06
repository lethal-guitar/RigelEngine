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
#include "data/player_model.hpp"
#include "engine/earth_quake_effect.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/texture.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player/components.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"

#include "game_mode.hpp"
#include "global_level_events.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <vector>

namespace rigel { namespace game_logic { class IngameSystems; }}


namespace rigel {

class GameRunner : public entityx::Receiver<GameRunner> {
public:
  GameRunner(
    data::PlayerModel* pPlayerModel,
    int episode,
    int level,
    data::Difficulty difficulty,
    GameMode::Context context,
    boost::optional<base::Vector> playerPositionOverride = boost::none);
  ~GameRunner();

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);

  bool levelFinished() const;
  void showWelcomeMessage();

  void receive(const events::ScreenFlash& event);
  void receive(const events::PlayerMessage& event);

private:
  void loadLevel(
    int episode,
    int levelNumber,
    data::Difficulty difficulty,
    const loader::ResourceLoader& resources
  );

  void handleLevelExit();
  void handlePlayerDeath();
  void restartLevel();
  void handleTeleporter();

  void showDebugText();

private:
  engine::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  entityx::EventManager mEventManager;
  entityx::EntityManager mEntities;
  game_logic::EntityFactory mEntityFactory;

  data::PlayerModel* mpPlayerModel;
  data::PlayerModel mPlayerModelAtLevelStart;
  base::Vector mScrollOffset;
  game_logic::PlayerInputState mInputState;
  game_logic::PlayerInputState mCombinedInputState;
  bool mLevelFinished;

  engine::TimeDelta mAccumulatedTime;

  bool mShowDebugText;
  bool mSingleStepping = false;
  bool mDoNextSingleStep = false;

  struct LevelData {
    data::map::Map mMap;
    std::vector<data::map::LevelData::Actor> mInitialActors;
    data::map::BackdropSwitchCondition mBackdropSwitchCondition;
  };

  LevelData mLevelData;
  data::map::Map mMapAtLevelStart;
  entityx::Entity mPlayerEntity;

  std::unique_ptr<game_logic::IngameSystems> mpSystems;

  engine::RandomNumberGenerator mRandomGenerator;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  engine::RenderTargetTexture mIngameViewPortRenderTarget;

  boost::optional<engine::EarthQuakeEffect> mEarthQuakeEffect;
  boost::optional<base::Color> mScreenFlashColor;
};

}
