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
#include <imfilebrowser.h>
RIGEL_RESTORE_WARNINGS


namespace rigel {
  struct IGameServiceProvider;
  class UserProfile;
}


namespace rigel::ui {

class OptionsMenu {
public:
  enum class Type {
    Main,
    InGame
  };

  OptionsMenu(
    UserProfile* pUserProfile,
    IGameServiceProvider* pServiceProvider,
    Type type);

  void updateAndRender(engine::TimeDelta dt);
  bool isFinished() const;

private:
  ImGui::FileBrowser mGamePathBrowser;
  UserProfile* mpUserProfile;
  data::GameOptions* mpOptions;
  IGameServiceProvider* mpServiceProvider;
  Type mType;
  bool mMenuOpen = true;
  bool mShowErrorBox = false;
};

}
