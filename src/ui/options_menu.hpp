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
#include "data/game_options.hpp"
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_events.h>
#include <imfilebrowser.h>
#include <imgui.h>
RIGEL_RESTORE_WARNINGS


namespace rigel
{
struct IGameServiceProvider;
class UserProfile;
} // namespace rigel

namespace rigel::renderer
{
class Renderer;
}


namespace rigel::ui
{

class OptionsMenu
{
public:
  enum class Type
  {
    Main,
    InGame
  };

  OptionsMenu(
    UserProfile* pUserProfile,
    IGameServiceProvider* pServiceProvider,
    renderer::Renderer* pRenderer,
    Type type);
  ~OptionsMenu();

  OptionsMenu(const OptionsMenu&) = delete;
  OptionsMenu& operator=(const OptionsMenu&) = delete;
  OptionsMenu(OptionsMenu&&) = default;
  OptionsMenu& operator=(OptionsMenu&&) = default;

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);
  bool isFinished() const;

private:
  void keyBindingRow(const char* label, SDL_Keycode* binding);
  void beginRebinding(SDL_Keycode* binding);
  void endRebinding();
  bool shouldDrawGamePathChooser() const;
  void drawGamePathChooser(const ImVec2& sizeToUse);
  void drawCreditsBox(engine::TimeDelta dt);

  ImGui::FileBrowser mGamePathBrowser;
  UserProfile* mpUserProfile;
  data::GameOptions* mpOptions;
  IGameServiceProvider* mpServiceProvider;
  renderer::Renderer* mpRenderer;

  int mSelectedSoundIndex = 0;

  SDL_Keycode* mpCurrentlyEditedBinding = nullptr;
  engine::TimeDelta mElapsedTimeEditingBinding = 0;
  engine::TimeDelta mElapsedTimeForCreditsBox = 0;
  float mGamePathChooserHeightNormalized = 0.0f;
  float mCreditsBoxContentHeightNormalized = 0.0f;

  Type mType;
  bool mMenuOpen = true;
  bool mPopupOpened = false;
  bool mShowErrorBox = false;
  bool mIsRunningInDesktopEnvironment;
};

} // namespace rigel::ui
