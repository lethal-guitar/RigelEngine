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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/player_data.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/texture.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player/components.hpp"
#include "ui/hud_renderer.hpp"

#include "game_mode.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <vector>


namespace rigel {

class IngameMode : public GameMode {
public:
  IngameMode(
    int episode,
    int level,
    data::Difficulty difficulty,
    Context context,
    boost::optional<base::Vector> playerPositionOverride = boost::none);
  ~IngameMode();

  void handleEvent(const SDL_Event& event) override;
  void updateAndRender(engine::TimeDelta dt) override;

  bool levelFinished() const;

private:
  void showLoadingScreen(int episode, const loader::ResourceLoader& resources);

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
  entityx::EntityX mEntities;
  game_logic::EntityFactory mEntityFactory;

  data::PlayerModel mPlayerModel;
  data::PlayerModel mPlayerModelAtLevelStart;
  base::Vector mScrollOffset;
  game_logic::PlayerInputState mPlayerInputs;
  bool mLevelFinished;

  bool mShowDebugText;

  struct LevelData {
    data::map::Map mMap;
    std::vector<data::map::LevelData::Actor> mInitialActors;
    data::map::BackdropSwitchCondition mBackdropSwitchCondition;
  };

  struct Systems;

  LevelData mLevelData;
  data::map::Map mMapAtLevelStart;
  entityx::Entity mPlayerEntity;

  std::unique_ptr<Systems> mpSystems;

  engine::RandomNumberGenerator mRandomGenerator;
  ui::HudRenderer mHudRenderer;
  engine::RenderTargetTexture mIngameViewPortRenderTarget;

  boost::optional<entityx::Entity> mActiveTeleporter;
};

}
