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

#include "version_info.hpp"

#include "base/math_tools.hpp"
#include "base/string_utils.hpp"
#include "base/warnings.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "sdl_utils/key_code.hpp"
#include "ui/menu_navigation.hpp"
#include "ui/utils.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_keyboard.h>
#include <SDL_mixer.h>
#include <imgui_internal.h>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <cassert>
#include <filesystem>


namespace rigel::ui
{

namespace
{

#ifdef RIGEL_USE_GL_ES
constexpr auto OPENGL_VARIANT = "OpenGL ES";
#else
constexpr auto OPENGL_VARIANT = "OpenGL";
#endif

constexpr auto WINDOW_SCALE = 0.8f;

constexpr auto MAX_OPTIONS_MENU_ASPECT_RATIO = 16.0f / 9.0f;

constexpr auto STANDARD_FPS_LIMITS =
  std::array<int, 8>{30, 60, 70, 72, 90, 120, 144, 240};


template <typename Callback>
void withEnabledState(const bool enabled, Callback&& callback)
{
  if (!enabled)
  {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  callback();

  if (!enabled)
  {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }
}


void fpsLimitUi(data::GameOptions* pOptions)
{
  withEnabledState(!pOptions->mEnableVsync, [=]() {
    if (pOptions->mEnableVsync)
    {
      // When V-Sync is on, we always want to show FPS limiting as off,
      // regardless of the actual setting in the options.
      bool alwaysFalse = false;
      ImGui::Checkbox("Limit max FPS", &alwaysFalse);
    }
    else
    {
      ImGui::Checkbox("Limit max FPS", &pOptions->mEnableFpsLimit);
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 3.8f);
    if (ImGui::BeginCombo("Limit", std::to_string(pOptions->mMaxFps).c_str()))
    {
      for (const auto item : STANDARD_FPS_LIMITS)
      {
        const auto isSelected = item == pOptions->mMaxFps;

        if (ImGui::Selectable(std::to_string(item).c_str(), isSelected))
        {
          pOptions->mMaxFps = item;
        }

        if (isSelected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }
  });
}


std::string normalizedKeyName(const SDL_Keycode keyCode)
{
  using namespace std::literals;

  constexpr auto variantPrefix = "Left "sv;

  auto keyName = std::string(SDL_GetKeyName(keyCode));

  if (strings::startsWith(keyName, variantPrefix))
  {
    keyName.erase(0, variantPrefix.size());
  }

  return keyName;
}


bool determineIfRunningInDesktopEnvironment()
{
  const auto sdlVideoDriver = std::string(SDL_GetCurrentVideoDriver());

  // clang-format off
  return
    sdlVideoDriver == "cocoa" ||
    sdlVideoDriver == "wayland" ||
    sdlVideoDriver == "windows" ||
    sdlVideoDriver == "x11";
  // clang-format on
}


ImVec2 clampAspectRatio(ImVec2 windowSize)
{
  return ImVec2{
    std::min(windowSize.x, windowSize.y * MAX_OPTIONS_MENU_ASPECT_RATIO),
    windowSize.y};
}


bool isSmallScreen(const ImVec2& windowSize)
{
  // On small screen resolutions, we want to make use of all available screen
  // space. We arbitrarily define anything lower than 800x600 as "small".
  // This is primarily for the OGA, which has a 480x320 screen.
  return windowSize.x < 800 || windowSize.y < 600;
}

} // namespace


OptionsMenu::OptionsMenu(
  UserProfile* pUserProfile,
  IGameServiceProvider* pServiceProvider,
  const Type type)
  : mGamePathBrowser(
      ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_CloseOnEsc)
  , mpUserProfile(pUserProfile)
  , mpOptions(&pUserProfile->mOptions)
  , mpServiceProvider(pServiceProvider)
  , mType(type)
  , mIsRunningInDesktopEnvironment(determineIfRunningInDesktopEnvironment())
{
  mGamePathBrowser.SetTitle("Choose Duke Nukem II installation");
}


OptionsMenu::~OptionsMenu()
{
  assert(!mpCurrentlyEditedBinding);
}


void OptionsMenu::handleEvent(const SDL_Event& event)
{
  if (
    !mpCurrentlyEditedBinding ||
    (event.type != SDL_KEYUP && event.type != SDL_KEYDOWN))
  {
    return;
  }

  const auto keyCode =
    sdl_utils::normalizeLeftRightVariants(event.key.keysym.sym);

  if (keyCode == SDLK_ESCAPE)
  {
    // We need to handle the key up, as ImGui would otherwise see the key up
    // event if we acted on key down. So we act on key up, and swallow the
    // key down event by always returning.
    if (event.type == SDL_KEYUP)
    {
      endRebinding();
    }

    return;
  }

  if (event.type == SDL_KEYDOWN && data::canBeUsedForKeyBinding(keyCode))
  {
    // Store the new key binding
    *mpCurrentlyEditedBinding = keyCode;

    // Unbind any duplicates
    for (auto pBinding : mpOptions->allKeyBindings())
    {
      if (pBinding != mpCurrentlyEditedBinding && *pBinding == keyCode)
      {
        *pBinding = SDLK_UNKNOWN;
      }
    }

    endRebinding();
  }
}


void OptionsMenu::updateAndRender(engine::TimeDelta dt)
{
  namespace fs = std::filesystem;

  if (mpCurrentlyEditedBinding)
  {
    mElapsedTimeEditingBinding += dt;
  }

  ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

  if (mPopupOpened && !ImGui::IsPopupOpen("Options"))
  {
    // Popup was closed, quit the options menu
    endRebinding();
    mMenuOpen = false;
    return;
  }

  if (mMenuOpen && !mPopupOpened)
  {
    ImGui::OpenPopup("Options");
    mPopupOpened = true;
  }

  auto soundStyleChanged = false;

  const auto& io = ImGui::GetIO();
  const auto windowSize = io.DisplaySize;

  const auto scaleX = isSmallScreen(windowSize) ? 1.0f : WINDOW_SCALE;
  const auto scaleY = isSmallScreen(windowSize) ? 1.0f : WINDOW_SCALE;

  const auto sizeToUse =
    clampAspectRatio(ImVec2{windowSize.x * scaleX, windowSize.y * scaleY});
  const auto offset = ImVec2{
    (windowSize.x - sizeToUse.x) / 2.0f, (windowSize.y - sizeToUse.y) / 2.0f};

  ImGui::SetNextWindowSize(sizeToUse);
  ImGui::SetNextWindowPos(offset);

  if (!ImGui::BeginPopup(
        "Options", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
  {
    return;
  }

  auto stopRebindingDueToTabSwitch = true;

  if (ImGui::BeginTabBar("Tabs"))
  {
    if (ImGui::BeginTabItem("Graphics"))
    {
      ImGui::NewLine();

      if (mIsRunningInDesktopEnvironment)
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
      ImGui::Checkbox(
        "Enable screen flashing", &mpOptions->mEnableScreenFlashes);

      {
        auto upscalingFilterIndex =
          static_cast<int>(mpOptions->mUpscalingFilter);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20);
        ImGui::Combo(
          "Upscaling filter",
          &upscalingFilterIndex,
          "None (nearest neighbor)\0Linear\0");
        mpOptions->mUpscalingFilter =
          static_cast<data::UpscalingFilter>(upscalingFilterIndex);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sound"))
    {
      ImGui::NewLine();

      auto soundEffectStyleIndex = static_cast<int>(mpOptions->mSoundStyle);
      ImGui::Combo(
        "Sound effects style",
        &soundEffectStyleIndex,
        "AdLib\0Sound Blaster\0Combined AdLib + SB\0");

      const auto newSoundStyle =
        static_cast<data::SoundStyle>(std::clamp(soundEffectStyleIndex, 0, 2));
      soundStyleChanged = newSoundStyle != mpOptions->mSoundStyle;
      mpOptions->mSoundStyle = newSoundStyle;

      const auto sliderWidth =
        std::min(sizeToUse.x / 2.0f, ImGui::GetFontSize() * 24);

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

    if (ImGui::BeginTabItem("Keyboard controls"))
    {
      stopRebindingDueToTabSwitch = false;

      ImGui::NewLine();
      ImGui::Columns(2);
      keyBindingRow("Up", &mpOptions->mUpKeybinding);
      keyBindingRow("Down", &mpOptions->mDownKeybinding);
      keyBindingRow("Left", &mpOptions->mLeftKeybinding);
      keyBindingRow("Right", &mpOptions->mRightKeybinding);
      keyBindingRow("Jump", &mpOptions->mJumpKeybinding);
      keyBindingRow("Fire", &mpOptions->mFireKeybinding);
      keyBindingRow("Quick save", &mpOptions->mQuickSaveKeybinding);
      keyBindingRow("Quick load", &mpOptions->mQuickLoadKeybinding);
      ImGui::Columns(1);

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Enhancements"))
    {
      ImGui::NewLine();

#if 0
      // NOTE: This is disabled for now, it's not quite ready yet to be made
      // user-facing.
      const auto canUseCompatibilityMode = !mpOptions->mWidescreenModeOn;

      withEnabledState(canUseCompatibilityMode, [&]() {
        auto gameplayStyleIndex =
          mpOptions->mCompatibilityModeOn && canUseCompatibilityMode ? 0 : 1;
        ImGui::Combo(
          "Gameplay style",
          &gameplayStyleIndex,
          "Compatibility mode\0Enhanced mode\0");

        if (canUseCompatibilityMode) {
          mpOptions->mCompatibilityModeOn = gameplayStyleIndex == 0;
        }
      });
#endif

      ImGui::Checkbox("Widescreen mode", &mpOptions->mWidescreenModeOn);
      ImGui::Checkbox("Quick saving", &mpOptions->mQuickSavingEnabled);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("About"))
    {
      ImGui::NewLine();

      ImGui::Text(
        "RigelEngine v%d.%d.%d (commit %s) - %s renderer",
        VERSION_MAJOR,
        VERSION_MINOR,
        VERSION_PATCH,
        COMMIT_HASH,
        OPENGL_VARIANT);

      SDL_version sdlVersion;
      SDL_GetVersion(&sdlVersion);

      const auto pSdlMixerVersion = Mix_Linked_Version();

      ImGui::Text(
        "Using SDL v%d.%d.%d - SDL Mixer v%d.%d.%d - %s & %s backends",
        sdlVersion.major,
        sdlVersion.minor,
        sdlVersion.patch,
        pSdlMixerVersion->major,
        pSdlMixerVersion->minor,
        pSdlMixerVersion->patch,
        SDL_GetCurrentVideoDriver(),
        SDL_GetCurrentAudioDriver());
    }

    ImGui::EndTabBar();
  }

  // If the user selects a key for rebinding, and then switches to a different
  // tab via the mouse/gamepad, stop rebinding. To implement that, we always
  // stop rebinding when any other tab aside from keyboard controls is visible.
  if (stopRebindingDueToTabSwitch)
  {
    endRebinding();
  }

  // If a game path was specified on the command line, don't show the game path
  // chooser.
  if (shouldDrawGamePathChooser())
  {
    if (mGamePathChooserHeightNormalized == 0.0f)
    {
      // Draw the chooser into an invisible window to figure out its size
      ImGui::BeginChild("#dummy", {0.0f, 0.0f});
      drawGamePathChooser(sizeToUse);
      mGamePathChooserHeightNormalized = ImGui::GetCursorPosY() / sizeToUse.y;
      ImGui::EndChild();
    }

    ImGui::SetCursorPosY(
      ImGui::GetContentRegionMax().y -
      mGamePathChooserHeightNormalized * sizeToUse.y);
    drawGamePathChooser(sizeToUse);

    if (!mShowErrorBox)
    {
      mGamePathBrowser.Display();
    }

    if (mGamePathBrowser.HasSelected())
    {
      const auto newGamePath = mGamePathBrowser.GetSelected();
      if (fs::exists(newGamePath / "NUKEM2.CMP"))
      {
        mGamePathBrowser.Close();
        mpServiceProvider->switchGamePath(newGamePath);
      }
      else
      {
        // Re-open the browser
        mGamePathBrowser.Open();

        ImGui::OpenPopup("Error");
        mShowErrorBox = true;
      }
    }

    const auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal("Error", &mShowErrorBox, flags))
    {
      ImGui::TextUnformatted(
        "No game data (file NUKEM2.CMP) found at chosen path!");
      if (ImGui::Button("Ok"))
      {
        mShowErrorBox = false;
      }
      ImGui::EndPopup();
    }
  }

  ImGui::EndPopup();

  if (soundStyleChanged)
  {
    ImGui::GetForegroundDrawList()->AddRectFilled(
      {0, 0}, windowSize, ui::toImgui({0, 0, 0, 128}));
    ui::drawLoadingScreenText();
  }
}


bool OptionsMenu::isFinished() const
{
  return !mMenuOpen;
}


void OptionsMenu::keyBindingRow(const char* label, SDL_Keycode* binding)
{
  ImGui::Text("%s", label);
  ImGui::NextColumn();

  const auto buttonSize = ImVec2{ImGui::GetFontSize() * 15, 0};

  ImGui::PushID(binding);

  if (mpCurrentlyEditedBinding != binding)
  {
    const auto keyName =
      *binding == SDLK_UNKNOWN ? "- Unassigned -" : normalizedKeyName(*binding);

    if (ImGui::Button(keyName.c_str(), buttonSize))
    {
      beginRebinding(binding);
    }
  }
  else
  {
    // While waiting for a keypress to rebind a selected key, display an
    // animated button
    const auto color = static_cast<float>(
      std::abs(std::sin(mElapsedTimeEditingBinding / 0.25) / 2.0) + 0.3);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color, 0, 0, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color, 0, 0, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color, 0, 0, 1.0f));

    if (ImGui::Button("- Press desired key -", buttonSize))
    {
      endRebinding();
    }

    ImGui::PopStyleColor(3);
  }

  ImGui::PopID();

  ImGui::NextColumn();
}


void OptionsMenu::beginRebinding(SDL_Keycode* binding)
{
  mpCurrentlyEditedBinding = binding;
  mElapsedTimeEditingBinding = 0.0;

  // In order to change a key binding, we need to receive key events. But
  // ImGui is intercepting them normally. To get around that, we temporarily
  // disable ImGui keyboard navigation while waiting for a key press.
  ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}


void OptionsMenu::endRebinding()
{
  mpCurrentlyEditedBinding = nullptr;
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}


bool OptionsMenu::shouldDrawGamePathChooser() const
{
  return mType == Type::Main &&
    mpServiceProvider->commandLineOptions().mGamePath.empty();
}


void OptionsMenu::drawGamePathChooser(const ImVec2& sizeToUse)
{
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (mpUserProfile->mGamePath)
  {
    ImGui::Text(
      "Current game path: '%s'", mpUserProfile->mGamePath->u8string().c_str());
    ImGui::Text(
      "Type: %s version",
      mpServiceProvider->isSharewareVersion() ? "Shareware" : "Registered");
  }

  ImGui::NewLine();
  if (ImGui::Button("Choose Duke Nukem II installation"))
  {
    if (mpUserProfile->mGamePath)
    {
      mGamePathBrowser.SetPwd(*mpUserProfile->mGamePath);
    }

    mGamePathBrowser.SetWindowSize(
      base::round(sizeToUse.x * 0.8f), base::round(sizeToUse.y * 0.8f));
    mGamePathBrowser.Open();
  }

  if (!mpServiceProvider->isSharewareVersion())
  {
    ImGui::Spacing();
    ImGui::TextUnformatted(
      R"(NOTE: When switching to a shareware version, some of your saved games
might become unusable.
Going back to a registered version will make them work again.)");
  }
}

} // namespace rigel::ui
