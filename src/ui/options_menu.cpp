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

#include "base/math_tools.hpp"
#include "base/warnings.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
#include <imgui_internal.h>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <filesystem>


namespace rigel::ui {

namespace {

constexpr auto SCALE = 0.8f;

constexpr auto STANDARD_FPS_LIMITS = std::array<int, 8>{
  30, 60, 70, 72, 90, 120, 144, 240};


template <typename Callback>
void withEnabledState(const bool enabled, Callback&& callback) {
  if (!enabled) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  callback();

  if (!enabled) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }
}


void fpsLimitUi(data::GameOptions* pOptions) {
  withEnabledState(!pOptions->mEnableVsync, [=]() {
    if (pOptions->mEnableVsync) {
      // When V-Sync is on, we always want to show FPS limiting as off,
      // regardless of the actual setting in the options.
      bool alwaysFalse = false;
      ImGui::Checkbox("Limit max FPS", &alwaysFalse);
    } else {
      ImGui::Checkbox("Limit max FPS", &pOptions->mEnableFpsLimit);
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 3.8f);
    if (ImGui::BeginCombo("Limit", std::to_string(pOptions->mMaxFps).c_str())) {
      for (const auto item : STANDARD_FPS_LIMITS) {
        const auto isSelected = item == pOptions->mMaxFps;

        if (ImGui::Selectable(std::to_string(item).c_str(), isSelected)) {
          pOptions->mMaxFps = item;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }
  });
}

}


OptionsMenu::OptionsMenu(
  UserProfile* pUserProfile,
  IGameServiceProvider* pServiceProvider,
  const Type type
)
  : mGamePathBrowser(
      ImGuiFileBrowserFlags_SelectDirectory |
      ImGuiFileBrowserFlags_CloseOnEsc)
  , mpUserProfile(pUserProfile)
  , mpOptions(&pUserProfile->mOptions)
  , mpServiceProvider(pServiceProvider)
  , mType(type)
{
  mGamePathBrowser.SetTitle("Choose Duke Nukem II installation");
}


void OptionsMenu::updateAndRender(engine::TimeDelta dt) {
  namespace fs = std::filesystem;

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
      {
        auto windowModeIndex = static_cast<int>(mpOptions->mWindowMode);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20);
        ImGui::Combo(
          "Window mode",
          &windowModeIndex,
          "Fullscreen (borderless)\0Exclusive fullscreen\0Windowed\0");
        mpOptions->mWindowMode = static_cast<data::WindowMode>(windowModeIndex);
      }

      ImGui::Checkbox("V-Sync on", &mpOptions->mEnableVsync);
      ImGui::SameLine();
      fpsLimitUi(mpOptions);
      ImGui::NewLine();

      ImGui::Checkbox("Show FPS", &mpOptions->mShowFpsCounter);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sound"))
    {
      const auto sliderWidth = ImGui::GetFontSize() * 24;

      ImGui::NewLine();
      ImGui::SetNextItemWidth(sliderWidth);
      ImGui::SliderFloat("Music volume", &mpOptions->mMusicVolume, 0.0f, 1.0f);
      ImGui::SameLine();
      ImGui::Checkbox("Music on", &mpOptions->mMusicOn);
      ImGui::NewLine();

      ImGui::SetNextItemWidth(sliderWidth);
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

  if (mType == Type::Main)
  {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (mpUserProfile->mGamePath) {
      ImGui::Text(
        "Current game path: '%s'",
        mpUserProfile->mGamePath->u8string().c_str());
      ImGui::Text(
        "Type: %s version",
        mpServiceProvider->isShareWareVersion() ? "Shareware" : "Registered");
    }

    ImGui::NewLine();
    if (ImGui::Button("Choose Duke Nukem II installation")) {
      if (mpUserProfile->mGamePath) {
        mGamePathBrowser.SetPwd(*mpUserProfile->mGamePath);
      }

      mGamePathBrowser.SetWindowSize(
        base::round(sizeToUse.x * 0.8f),
        base::round(sizeToUse.y * 0.8f));
      mGamePathBrowser.Open();
    }

    if (!mpServiceProvider->isShareWareVersion()) {
      ImGui::Spacing();
      ImGui::TextUnformatted(
R"(NOTE: When switching to a shareware version, some of your saved games
might become unusable.
Going back to a registered version will make them work again.)");
    }

    if (!mShowErrorBox) {
      mGamePathBrowser.Display();
    }

    if (mGamePathBrowser.HasSelected())
    {
      const auto newGamePath = mGamePathBrowser.GetSelected();
      if (fs::exists(newGamePath / "NUKEM2.CMP")) {
        mGamePathBrowser.Close();
        mpServiceProvider->switchGamePath(newGamePath);
      } else {
        // Re-open the browser
        mGamePathBrowser.Open();

        ImGui::OpenPopup("Error");
        mShowErrorBox = true;
      }
    }

    const auto flags =
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Error", &mShowErrorBox, flags)) {
      ImGui::TextUnformatted(
        "No game data (file NUKEM2.CMP) found at chosen path!");
      if (ImGui::Button("Ok")) {
        mShowErrorBox = false;
      }
      ImGui::EndPopup();
    }
  }

  ImGui::End();
}


bool OptionsMenu::isFinished() const {
  return !mMenuOpen;
}

}
