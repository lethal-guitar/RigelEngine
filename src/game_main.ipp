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
#include "sdl_utils/ptr.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/fps_display.hpp"
#include "ui/menu_element_renderer.hpp"

#include "SDL_gamecontroller.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>


namespace rigel {

class FpsLimiter {
public:
  explicit FpsLimiter(int targetFps);

  void updateAndWait();

private:
  std::chrono::high_resolution_clock::time_point mLastTime = {};
  double mTargetFrameTime;
  double mError = 0.0;
};


class Game : public IGameServiceProvider {
public:
  enum class RunResult {
    GameEnded,
    RestartNeeded
  };

  Game(
    const CommandLineOptions& commandLineOptions,
    UserProfile* pUserProfile,
    SDL_Window* pWindow);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  RunResult run();

private:
  enum class FadeType {
    In,
    Out
  };

  RunResult mainLoop();
  void pumpEvents(std::vector<SDL_Event>& eventQueue);
  void updateAndRender(
    entityx::TimeDelta elapsed,
    const std::vector<SDL_Event>& eventQueue);

  GameMode::Context makeModeContext();

  bool handleEvent(const SDL_Event& event);

  void performScreenFadeBlocking(FadeType type);

  void swapBuffers();
  void applyChangedOptions();
  void enumerateGameControllers();

  // IGameServiceProvider implementation
  void fadeOutScreen() override;
  void fadeInScreen() override;
  void playSound(data::SoundId id) override;
  void stopSound(data::SoundId id) override;
  void playMusic(const std::string& name) override;
  void stopMusic() override;

  void scheduleGameQuit() override;

  void switchGamePath(const std::filesystem::path& newGamePath) override;

  bool isShareWareVersion() const override {
    return mIsShareWareVersion;
  }

  const CommandLineOptions& commandLineOptions() const override {
    return mCommandLineOptions;
  }

private:
  SDL_Window* mpWindow;
  renderer::Renderer mRenderer;
  engine::SoundSystem mSoundSystem;
  loader::ResourceLoader mResources;
  bool mIsShareWareVersion;

  std::optional<FpsLimiter> mFpsLimiter;
  renderer::RenderTargetTexture mRenderTarget;
  std::uint8_t mAlphaMod = 255;

  std::unique_ptr<GameMode> mpCurrentGameMode;

  std::vector<engine::SoundSystem::SoundHandle> mSoundsById;

  bool mIsRunning;
  bool mIsMinimized;
  std::chrono::high_resolution_clock::time_point mLastTime;

  CommandLineOptions mCommandLineOptions;
  UserProfile* mpUserProfile;
  data::GameOptions mPreviousOptions;
  std::filesystem::path mGamePathToSwitchTo;

  ui::DukeScriptRunner mScriptRunner;
  loader::ScriptBundle mAllScripts;
  engine::TiledTexture mUiSpriteSheet;
  ui::MenuElementRenderer mTextRenderer;
  ui::FpsDisplay mFpsDisplay;
  sdl_utils::Ptr<SDL_GameController> mpGameController;
};

}
