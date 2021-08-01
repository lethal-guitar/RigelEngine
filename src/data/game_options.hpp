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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_keycode.h>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <cstdint>
#include <optional>


namespace rigel::data
{

// In the majority of cases, the value of an option can be re-evaluated every
// frame, and whatever code implements the option can adjust accordingly.
// But some options require additional action to make them effective, e.g.
// the v-sync option has to be applied by calling SDL_GL_SetSwapInterval.
//
// For these options, you should define their default value here, and make use
// of that constant in the code that applies the setting initially. The v-sync
// default value is used in renderer/renderer.cpp, for example.
//
// To react to changes for options that require additional actions, look at
// Game::applyChangedOptions() in game_main.cpp.
constexpr auto ENABLE_VSYNC_DEFAULT = true;
constexpr auto MUSIC_VOLUME_DEFAULT = 1.0f;
constexpr auto SOUND_VOLUME_DEFAULT = 1.0f;

enum class WindowMode
{
  Fullscreen,
  ExclusiveFullscreen,
  Windowed
};

#if defined(__EMSCRIPTEN__)
constexpr auto DEFAULT_WINDOW_MODE = WindowMode::Windowed;
#elif defined(__APPLE__) || defined(RIGEL_USE_GL_ES)
constexpr auto DEFAULT_WINDOW_MODE = WindowMode::ExclusiveFullscreen;
#else
constexpr auto DEFAULT_WINDOW_MODE = WindowMode::Fullscreen;
#endif

enum class SoundStyle : std::uint8_t
{
  AdLib,
  SoundBlaster,
  Combined
};


enum class UpscalingFilter : std::uint8_t
{
  None,
  Linear
};


/** Data-model for user-configurable options/settings
 *
 * This struct contains everything that can be configured by the user in
 * RigelEngine. The corresponding UI is located in ui/options_menu.cpp
 * Serialization code is found in common/user_profile.cpp
 *
 * If you add something to this struct, you most likely want to add
 * serialization and UI as well!
 */
struct GameOptions
{
  // Graphics
  WindowMode mWindowMode = DEFAULT_WINDOW_MODE;

  // Note: These are not meant to be directly changed by the user. Instead,
  // they are automatically updated every time the window is moved or resized
  // when in windowed mode. This way, the window's position and size will be
  // remembered until next time.
  bool mWindowCoordsValid = false;
  int mWindowPosX = 0;
  int mWindowPosY = 0;
  int mWindowWidth = 1920;
  int mWindowHeight = 1080;

  bool mEnableVsync = ENABLE_VSYNC_DEFAULT;
  bool mEnableFpsLimit = true; // Only relevant when mEnableVsync == false
  int mMaxFps = 60; // Only relevant when mEnableFpsLimit == true
  bool mShowFpsCounter = false;
  bool mEnableScreenFlashes = true;
  UpscalingFilter mUpscalingFilter = UpscalingFilter::None;

  // Sound
  float mMusicVolume = MUSIC_VOLUME_DEFAULT;
  float mSoundVolume = SOUND_VOLUME_DEFAULT;
  bool mMusicOn = true;
  bool mSoundOn = true;
  SoundStyle mSoundStyle = SoundStyle::SoundBlaster;

  // Keyboard controls
  SDL_Keycode mUpKeybinding = SDLK_UP;
  SDL_Keycode mDownKeybinding = SDLK_DOWN;
  SDL_Keycode mLeftKeybinding = SDLK_LEFT;
  SDL_Keycode mRightKeybinding = SDLK_RIGHT;
  SDL_Keycode mJumpKeybinding = SDLK_LCTRL;
  SDL_Keycode mFireKeybinding = SDLK_LALT;
  SDL_Keycode mQuickSaveKeybinding = SDLK_F5;
  SDL_Keycode mQuickLoadKeybinding = SDLK_F7;

  // Gameplay
  bool mCompatibilityModeOn = false;

  // Enhancements
  bool mWidescreenModeOn = false;
  bool mQuickSavingEnabled = false;

  // Internal options
  //
  // The following options are used internally to control various behavior, but
  // they are not serialized to disk and not editable by the user. The game
  // sets these options itself.
  bool mPerElementUpscalingEnabled = false;

  // Helper functions
  bool compatibilityModeOn() const
  {
    return mCompatibilityModeOn && !mWidescreenModeOn;
  }

  std::array<SDL_Keycode*, 8> allKeyBindings()
  {
    return {
      &mUpKeybinding,
      &mDownKeybinding,
      &mLeftKeybinding,
      &mRightKeybinding,
      &mJumpKeybinding,
      &mFireKeybinding,
      &mQuickSaveKeybinding,
      &mQuickLoadKeybinding,
    };
  }
};


bool canBeUsedForKeyBinding(SDL_Keycode key);

} // namespace rigel::data
