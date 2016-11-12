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

#include <base/spatial_types.hpp>
#include <data/player_data.hpp>
#include <game_logic/entity_factory.hpp>
#include <game_logic/player_control_system.hpp>
#include <sdl_utils/texture.hpp>
#include <ui/hud_renderer.hpp>

#include <game_mode.hpp>

#include <entityx/entityx.h>
#include <SDL.h>
#include <vector>


namespace rigel {

class IngameMode : public GameMode {
public:
  IngameMode(
    int episode,
    int level,
    data::Difficulty difficulty,
    Context context);

  void handleEvent(const SDL_Event& event) override;
  void updateAndRender(engine::TimeDelta dt) override;

private:
  void showLoadingScreen(int episode, const loader::ResourceLoader& resources);

  void loadLevel(
    int episode,
    int levelNumber,
    data::Difficulty difficulty,
    const loader::ResourceLoader& resources
  );

private:
  SDL_Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  data::PlayerModel mPlayerModel;
  base::Vector mScrollOffset;
  game_logic::PlayerInputState mPlayerInputs;

  entityx::EntityX mEntities;
  entityx::Entity mPlayerEntity;

  ui::HudRenderer mHudRenderer;
  std::vector<sdl_utils::OwningTexture> mSpriteTextures;
  sdl_utils::RenderTargetTexture mIngameViewPortRenderTarget;
};

}
