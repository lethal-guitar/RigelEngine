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
#include "common/game_mode.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "engine/sound_system.hpp"
#include "engine/tiled_texture.hpp"
#include "loader/duke_script_loader.hpp"
#include "loader/resource_loader.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/fps_display.hpp"
#include "ui/menu_element_renderer.hpp"

#include <chrono>
#include <memory>
#include <string>


namespace rigel {


class Game : public IGameServiceProvider {
public:
  Game(
    const std::string& gamePath,
    UserProfile* pUserProfile,
    SDL_Window* pWindow);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  void run(const StartupOptions& options);

private:
  enum class FadeType {
    In,
    Out
  };

  void mainLoop();
  void pumpEvents(std::vector<SDL_Event>& eventQueue);

  GameMode::Context makeModeContext();

  bool handleEvent(const SDL_Event& event);

  void performScreenFadeBlocking(FadeType type);

  void applyChangedOptions();

  // IGameServiceProvider implementation
  void fadeOutScreen() override;
  void fadeInScreen() override;
  void playSound(data::SoundId id) override;
  void stopSound(data::SoundId id) override;
  void playMusic(const std::string& name) override;
  void stopMusic() override;

  void scheduleGameQuit() override;

  bool isShareWareVersion() const override {
    return mIsShareWareVersion;
  }

private:
  SDL_Window* mpWindow;
  renderer::Renderer mRenderer;
  engine::SoundSystem mSoundSystem;
  loader::ResourceLoader mResources;
  bool mIsShareWareVersion;

  renderer::RenderTargetTexture mRenderTarget;
  std::uint8_t mAlphaMod = 255;

  std::unique_ptr<GameMode> mpCurrentGameMode;
  std::unique_ptr<GameMode> mpNextGameMode;

  std::vector<engine::SoundSystem::SoundHandle> mSoundsById;

  bool mIsRunning;
  bool mIsMinimized;
  std::chrono::high_resolution_clock::time_point mLastTime;

  UserProfile* mpUserProfile;
  data::GameOptions mPreviousOptions;

  ui::DukeScriptRunner mScriptRunner;
  loader::ScriptBundle mAllScripts;
  engine::TiledTexture mUiSpriteSheet;
  ui::MenuElementRenderer mTextRenderer;
  ui::FpsDisplay mFpsDisplay;
};

}
