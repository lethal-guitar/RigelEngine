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

// In this file, we define the UI for the game's options menu. When adding
// a new member to the data::GameOptions struct, you most likely want to add
// corresponding UI code here as well!

#include "options_menu.hpp"

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::ui {

namespace {

constexpr auto SCALE = 0.8f;

}


void OptionsMenu::updateAndRender(engine::TimeDelta dt) {
  ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

  const auto& io = ImGui::GetIO();

  const auto windowSize = io.DisplaySize;
  const auto sizeToUse = ImVec2{windowSize.x * SCALE, windowSize.y * SCALE};
  const auto offset = ImVec2{
    (windowSize.x - sizeToUse.x) / 2.0f,
    (windowSize.y - sizeToUse.y) / 2.0f};

  ImGui::SetNextWindowSize(sizeToUse);
  ImGui::SetNextWindowPos(offset);
  ImGui::Begin(
    "Options",
    &mMenuOpen,
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoResize);

  if (ImGui::BeginTabBar("Tabs"))
  {
    if (ImGui::BeginTabItem("Graphics"))
    {
      ImGui::NewLine();
      ImGui::Checkbox("V-Sync on", &mpOptions->mEnableVsync);
      ImGui::Checkbox("Show FPS", &mpOptions->mShowFpsCounter);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sound"))
    {
      ImGui::NewLine();
      ImGui::SliderFloat("Music volume", &mpOptions->mMusicVolume, 0.0f, 1.0f);
      ImGui::SameLine();
      ImGui::Checkbox("Music on", &mpOptions->mMusicOn);
      ImGui::NewLine();

      ImGui::SliderFloat("Sound volume", &mpOptions->mSoundVolume, 0.0f, 1.0f);
      ImGui::SameLine();
      ImGui::Checkbox("Sound on", &mpOptions->mSoundOn);
      ImGui::NewLine();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Enhancements"))
    {
      ImGui::NewLine();
      ImGui::Checkbox("Widescreen mode", &mpOptions->mWidescreenModeOn);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}


bool OptionsMenu::isFinished() const {
  return !mMenuOpen;
}

}
