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

#include "assets/duke_script_loader.hpp"
#include "base/warnings.hpp"
#include "engine/timing.hpp"
#include "renderer/renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_events.h>
RIGEL_RESTORE_WARNINGS

#include <memory>
#include <string>


namespace rigel
{

struct IGameServiceProvider;
class UserProfile;

namespace engine
{
class SpriteFactory;
class TiledTexture;
} // namespace engine

namespace assets
{
class ResourceLoader;
}

namespace ui
{
class MenuElementRenderer;
class DukeScriptRunner;
} // namespace ui


struct GameMode
{
  /** Contains everything a mode needs */
  struct Context
  {
    const assets::ResourceLoader* mpResources;
    renderer::Renderer* mpRenderer;
    IGameServiceProvider* mpServiceProvider;
    ui::DukeScriptRunner* mpScriptRunner;
    assets::ScriptBundle* mpScripts;
    ui::MenuElementRenderer* mpUiRenderer;
    engine::TiledTexture* mpUiSpriteSheet;
    engine::SpriteFactory* mpSpriteFactory;
    UserProfile* mpUserProfile;
  };

  virtual ~GameMode() = default;

  virtual std::unique_ptr<GameMode> updateAndRender(
    engine::TimeDelta dt,
    const std::vector<SDL_Event>& events) = 0;

  virtual bool needsPerElementUpscaling() const { return false; }
};


void runScript(GameMode::Context& context, const std::string& scriptName);

} // namespace rigel
