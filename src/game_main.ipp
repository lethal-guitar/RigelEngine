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
#include "engine/renderer.hpp"
#include "engine/sound_system.hpp"
#include "engine/texture.hpp"
#include "loader/resource_loader.hpp"
#include "ui/fps_display.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/menu_element_renderer.hpp"

#include "game_mode.hpp"
#include "game_options.hpp"
#include "game_service_provider.hpp"

#include <chrono>
#include <memory>
#include <string>


namespace rigel {


class Game : public IGameServiceProvider {
public:
  Game(const std::string& gamePath, SDL_Window* pWindow);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  void run(const StartupOptions& options);

private:
  void showAntiPiracyScreen();

  void mainLoop();

  GameMode::Context makeModeContext();

  void handleEvent(const SDL_Event& event);

  void performScreenFadeBlocking(bool doFadeIn);

  // IGameServiceProvider implementation
  void fadeOutScreen() override;
  void fadeInScreen() override;
  void playSound(data::SoundId id) override;
  void stopSound(data::SoundId id) override;
  void playMusic(const std::string& name) override;
  void stopMusic() override;

  void scheduleNewGameStart(
    int episode,
    data::Difficulty difficulty) override;
  void scheduleEnterMainMenu() override;
  void scheduleGameQuit() override;

  bool isShareWareVersion() const override {
    return mIsShareWareVersion;
  }

  void showDebugText(const std::string& text) override;

private:
  engine::Renderer mRenderer;
  engine::SoundSystem mSoundSystem;
  loader::ResourceLoader mResources;
  bool mIsShareWareVersion;

  engine::RenderTargetTexture mRenderTarget;

  std::unique_ptr<GameMode> mpCurrentGameMode;
  std::unique_ptr<GameMode> mpNextGameMode;

  std::vector<engine::SoundSystem::SoundHandle> mSoundsById;

  GameOptions mOptions;
  bool mMusicEnabled = true;

  bool mIsRunning;
  bool mIsMinimized;
  std::chrono::high_resolution_clock::time_point mLastTime;

  ui::DukeScriptRunner mScriptRunner;
  ui::MenuElementRenderer mTextRenderer;
  ui::FpsDisplay mFpsDisplay;
  std::string mDebugText;
};

}
