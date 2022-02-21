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
#include "assets/resource_loader.hpp"
#include "audio/sound_system.hpp"
#include "base/clock.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/sprite_factory.hpp"
#include "engine/tiled_texture.hpp"
#include "frontend/game_mode.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "renderer/fps_limiter.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"
#include "sdl_utils/ptr.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/fps_display.hpp"
#include "ui/menu_element_renderer.hpp"

#include <SDL_gamecontroller.h>

#include <memory>
#include <optional>
#include <string>


namespace rigel
{

class Game : public IGameServiceProvider
{
public:
  enum class StopReason
  {
    GameEnded,
    RestartNeeded
  };

  Game(
    const CommandLineOptions& commandLineOptions,
    UserProfile* pUserProfile,
    SDL_Window* pWindow,
    bool isFirstLaunch);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  /** Run one frame of the game
   *
   * Should be called in an infinite loop to implement the game's main loop,
   * or given as a callback to environments which own the main loop like
   * Emscripten.
   *
   * If an empty optional is returned, the game wants to keep running,
   * otherwise, the loop should be terminated. If the reason for stopping is
   * RestartNeeded, the game would like a new Game to be started after
   * terminating the loop, otherwise, the game is done and the whole program
   * can be terminated.
   */
  std::optional<StopReason> runOneFrame();

private:
  enum class FadeType
  {
    In,
    Out
  };

  void pumpEvents();
  void updateAndRender(entityx::TimeDelta elapsed);

  GameMode::Context makeModeContext();

  bool handleEvent(const SDL_Event& event);

  void performScreenFadeBlocking(FadeType type);

  void swapBuffers();
  void applyChangedOptions();
  void enumerateGameControllers();
  void takeScreenshot();
  void setPerElementUpscalingEnabled(bool enabled);

  // IGameServiceProvider implementation
  void fadeOutScreen() override;
  void fadeInScreen() override;
  void playSound(data::SoundId id) override;
  void stopSound(data::SoundId id) override;
  void stopAllSounds() override;
  void playMusic(const std::string& name) override;
  void stopMusic() override;

  void scheduleGameQuit() override;

  void switchGamePath(const std::filesystem::path& newGamePath) override;

  void markCurrentFrameAsWidescreen() override
  {
    mCurrentFrameIsWidescreen = true;
  }

  bool isSharewareVersion() const override { return mIsShareWareVersion; }

  const CommandLineOptions& commandLineOptions() const override
  {
    return mCommandLineOptions;
  }

  const GameControllerInfo& gameControllerInfo() const override
  {
    return mGameControllerInfo;
  }

private:
  SDL_Window* mpWindow;
  renderer::Renderer mRenderer;
  assets::ResourceLoader mResources;
  std::unique_ptr<audio::SoundSystem> mpSoundSystem;
  bool mIsShareWareVersion;

  std::optional<renderer::FpsLimiter> mFpsLimiter;
  renderer::RenderTargetTexture mRenderTarget;
  std::uint8_t mAlphaMod = 0;
  bool mCurrentFrameIsWidescreen = false;

  std::unique_ptr<GameMode> mpCurrentGameMode;

  bool mIsRunning;
  bool mIsMinimized;
  bool mScreenshotRequested = false;
  base::Clock::time_point mLastTime;

  CommandLineOptions mCommandLineOptions;
  UserProfile* mpUserProfile;
  data::GameOptions mPreviousOptions;
  base::Size<int> mPreviousWindowSize;
  bool mWidescreenModeWasActive;
  std::filesystem::path mGamePathToSwitchTo;

  ui::DukeScriptRunner mScriptRunner;
  assets::ScriptBundle mAllScripts;
  engine::TiledTexture mUiSpriteSheet;
  engine::SpriteFactory mSpriteFactory;
  ui::MenuElementRenderer mTextRenderer;
  ui::FpsDisplay mFpsDisplay;
  std::vector<SDL_Event> mEventQueue;

  GameControllerInfo mGameControllerInfo;
};

} // namespace rigel
