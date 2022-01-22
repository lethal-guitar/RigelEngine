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

#include "base/math_utils.hpp"
#include "base/string_utils.hpp"
#include "base/warnings.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "sdl_utils/key_code.hpp"
#include "sdl_utils/platform.hpp"
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

constexpr auto CREDITS_SCROLL_SPEED = 1.6f;

constexpr auto MAX_OPTIONS_MENU_ASPECT_RATIO = 16.0f / 9.0f;

constexpr auto STANDARD_FPS_LIMITS =
  std::array{30, 60, 70, 72, 75, 90, 120, 144, 240};


struct SoundIdWithDescription
{
  data::SoundId mId;
  const char* mDescription;
};


constexpr auto SOUND_IDS_FOR_SOUND_TEST =
  std::array<SoundIdWithDescription, 29>{
    {{data::SoundId::DukeNormalShot, "Duke's regular gun"},
     {data::SoundId::BigExplosion, "Big explosion"},
     {data::SoundId::DukePain, "Duke taking damage"},
     {data::SoundId::DukeDeath, "Duke's death"},
     {data::SoundId::Explosion, "Explosion 1"},
     {data::SoundId::GlassBreaking, "Glass breaking"},
     {data::SoundId::DukeLaserShot, "Duke's laser gun"},
     {data::SoundId::ItemPickup, "Item picked up"},
     {data::SoundId::WeaponPickup, "Weapon picked up"},
     {data::SoundId::EnemyHit, "Enemy hit"},
     {data::SoundId::Swoosh, "Swoosh"},
     {data::SoundId::FlameThrowerShot, "Duke's flame thrower"},
     {data::SoundId::DukeJumping, "Duke jumping"},
     {data::SoundId::LavaFountain, "Lava fountain"},
     {data::SoundId::DukeLanding, "Duke landing"},
     {data::SoundId::DukeAttachClimbable, "Duke grabbing pipe"},
     {data::SoundId::HammerSmash, "Hammer smash"},
     {data::SoundId::BlueKeyDoorOpened, "Door opened"},
     {data::SoundId::AlternateExplosion, "Explosion 2"},
     {data::SoundId::WaterDrop, "Water drop"},
     {data::SoundId::ForceFieldFizzle, "Force field fizzle"},
     {data::SoundId::SlidingDoor, "Sliding door"},
     {data::SoundId::FallingRock, "Falling rock"},
     {data::SoundId::EnemyLaserShot, "Enemy's laser gun"},
     {data::SoundId::EarthQuake, "Earthquake"},
     {data::SoundId::BiologicalEnemyDestroyed, "Enemy death"},
     {data::SoundId::Teleport, "Teleporter"},
     {data::SoundId::HealthPickup, "Health picked up"},
     {data::SoundId::LettersCollectedCorrectly, "'NUKEM' bonus"}}};


constexpr int stringLength(const char* string)
{
  int result = 0;
  while (*string++ != '\0')
  {
    ++result;
  }

  return result;
}


constexpr auto LONGEST_SOUND_DESCRIPTION_INDEX = []() {
  auto longestIndex = 0;
  auto longestLength = 0;

  for (auto i = 0u; i < SOUND_IDS_FOR_SOUND_TEST.size(); ++i)
  {
    const auto length = stringLength(SOUND_IDS_FOR_SOUND_TEST[i].mDescription);
    if (length > longestLength)
    {
      longestIndex = int(i);
      longestLength = length;
    }
  }

  return longestIndex;
}();


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
  , mIsRunningInDesktopEnvironment(sdl_utils::isRunningInDesktopEnvironment())
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

  using namespace std::string_literals;

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
          "None (nearest neighbor)\0Bilinear\0");
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

      auto adlibPlaybackTypeIndex =
        static_cast<int>(mpOptions->mAdlibPlaybackType);
      ImGui::Combo(
        "AdLib emulation", &adlibPlaybackTypeIndex, "DBOPL\0Nuked OPL3\0");

      const auto newAdlibPlaybackType = static_cast<data::AdlibPlaybackType>(
        std::clamp(adlibPlaybackTypeIndex, 0, 1));
      soundStyleChanged = soundStyleChanged ||
        newAdlibPlaybackType != mpOptions->mAdlibPlaybackType;
      mpOptions->mAdlibPlaybackType = newAdlibPlaybackType;

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

      const auto& selectedSound = SOUND_IDS_FOR_SOUND_TEST[mSelectedSoundIndex];

      if (ImGui::Button("Test sound"))
      {
        mpServiceProvider->playSound(selectedSound.mId);
      }

      ImGui::SameLine();

      if (ImGui::Button("<"))
      {
        --mSelectedSoundIndex;
        if (mSelectedSoundIndex < 0)
        {
          mSelectedSoundIndex = int(SOUND_IDS_FOR_SOUND_TEST.size()) - 1;
        }
      }

      ImGui::SameLine();

      const auto longestDescription =
        SOUND_IDS_FOR_SOUND_TEST[LONGEST_SOUND_DESCRIPTION_INDEX].mDescription +
        " (#99)"s;
      const auto longestDescriptionSize =
        ImGui::CalcTextSize(longestDescription.c_str()).x;

      const auto description = selectedSound.mDescription + " (#"s +
        std::to_string(static_cast<int>(selectedSound.mId) + 1) + ")";

      const auto descriptionSize = ImGui::CalcTextSize(description.c_str()).x;
      const auto spacing = (longestDescriptionSize - descriptionSize) / 2.0f;

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing);
      ImGui::TextUnformatted(description.c_str());
      ImGui::SameLine();

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing);

      if (ImGui::Button(">"))
      {
        ++mSelectedSoundIndex;
        mSelectedSoundIndex %= int(SOUND_IDS_FOR_SOUND_TEST.size());
      }

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

    if (ImGui::BeginTabItem("Gamepad controls"))
    {
      const auto& info = mpServiceProvider->gameControllerInfo();

      ImGui::NewLine();
      ImGui::TextUnformatted("Gamepad configuration options coming soon.");
      ImGui::TextUnformatted("For now, this just shows all detected gamepads.");
      ImGui::NewLine();

      if (info.mGameControllers.empty())
      {
        ImGui::TextUnformatted("No gamepads detected.");
      }
      else
      {
        ImGui::TextUnformatted("Detected gamepads:");
        ImGui::Spacing();

        for (const auto& controller : info.mGameControllers)
        {
          ImGui::Bullet();
          ImGui::TextUnformatted(SDL_GameControllerName(controller.get()));
        }
      }

      if (!info.mUnrecognizedControllers.empty())
      {
        ImGui::NewLine();

        ImGui::TextUnformatted("Unrecognized gamepads (no SDL mappings):");
        ImGui::Spacing();

        for (const auto& controller : info.mUnrecognizedControllers)
        {
          ImGui::Bullet();
          ImGui::Text(
            "%s (%s)", controller.mName.c_str(), controller.mGuid.c_str());
        }
      }

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
      ImGui::Checkbox("Skip intro sequence", &mpOptions->mSkipIntro);
      ImGui::Checkbox(
        "Smooth scrolling & movement (experimental)",
        &mpOptions->mMotionSmoothing);
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

      ImGui::Spacing();

      drawCreditsBox(dt);

      ImGui::EndTabItem();
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


void OptionsMenu::drawCreditsBox(const engine::TimeDelta dt)
{
  const auto windowSize = ImGui::GetWindowSize();
  const auto creditsBoxWidth =
    windowSize.x * (isSmallScreen(windowSize) ? 1.0f : 0.75f);
  const auto creditsBoxHeight = ImGui::GetContentRegionAvail().y -
    (shouldDrawGamePathChooser()
       ? mGamePathChooserHeightNormalized * windowSize.y
       : 0.0f);

  mElapsedTimeForCreditsBox += dt;

  const auto totalScrollOffset = static_cast<float>(
    mElapsedTimeForCreditsBox * CREDITS_SCROLL_SPEED * ImGui::GetFontSize());

  const auto totalContentHeight =
    mCreditsBoxContentHeightNormalized * windowSize.y;

  // We only know the content height after drawing the first frame, hence
  // we cannot determine the scroll position before knowing the height.
  const auto scrollPos = totalContentHeight != 0.0f
    ? std::fmod(totalScrollOffset, totalContentHeight)
    : 0.0f;

  auto centeredText = [&](const char* text) {
    const auto textSize = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((creditsBoxWidth - textSize) / 2.0f);
    ImGui::TextUnformatted(text);
  };

  auto centeredTextBig = [&](const char* text, const float factor = 1.8f) {
    auto pFont = ImGui::GetFont();
    const auto currentScale = pFont->Scale;

    pFont->Scale *= factor;
    ImGui::PushFont(pFont);
    centeredText(text);

    pFont->Scale = currentScale;
    ImGui::PopFont();
  };


  const auto offset =
    isSmallScreen(windowSize) ? 0.0f : (windowSize.x - creditsBoxWidth) / 2.0f;

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
  ImGui::SetNextWindowPos(
    {ImGui::GetCursorScreenPos().x + offset, ImGui::GetCursorScreenPos().y});

  ImGui::SetNextWindowScroll({-1.0f, scrollPos});
  ImGui::BeginChild(
    "#credits",
    {creditsBoxWidth, creditsBoxHeight},
    true,
    ImGuiWindowFlags_NoScrollbar);

  // Blank space at the beginning
  ImGui::SetCursorPosY(creditsBoxHeight);

  centeredTextBig("RigelEngine", 2.2f);
  ImGui::Spacing();
  centeredText("A modern re-implementation of the game Duke Nukem II");
  centeredText("(originally released in 1993 by Apogee Software)");

  ImGui::NewLine();

  centeredText("Created by Nikolai Wuttke (https://github.com/lethal_guitar)");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("With major contributions from");
  ImGui::Spacing();
  centeredText("Alain Martin (https://github.com/McMartin)");
  centeredText("Andrei-Florin Bencsik (https://github.com/bencsikandrei)");
  centeredText("Pratik Anand (https://github.com/pratikone)");
  centeredText("Ryan Brown (https://github.com/rbrown46)");
  centeredText("Soham Roy (https://github.com/sohamroy19)");
  centeredText("PatriotRossii (https://github.com/PatriotRossii)");
  centeredText("s-martin (https://github.com/s-martin)");

  ImGui::NewLine();

  centeredText("RigelEngine icon by LunarLoony (https://lunarloony.co.uk)");

  ImGui::NewLine();

  centeredText("openSUSE package by mnhauke (https://github.com/mnhauke)");

  ImGui::NewLine();

  centeredText("Many thanks to everyone else who contributed to the project!");
  centeredText("See AUTHORS.md on GitHub for a full list of contributors.");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Special thanks");
  ImGui::Spacing();
  centeredText("Apogee Software");
  centeredText(
    "Shikadi Modding Wiki (https://moddingwiki.shikadi.net/wiki/Main_Page)");
  centeredText("The DOSBox project");
  centeredText("IDA Pro disassembler by Hex Rays");
  centeredText("Anton Gulenko");
  centeredText(
    "Clint Basinger aka LGR (https://www.youtube.com/c/Lazygamereviews)");
  centeredText("Bart aka Dosgamert (https://www.youtube.com/c/dosgamert)");
  centeredText("MaxiTaxi Creative (https://www.instagram.com/maxitaxi.f500)");
  centeredText("Everyone on the RigelEngine Discord");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredText("Rigel Engine Copyright (C) 2016, Nikolai Wuttke.");
  centeredText("Rigel Engine comes with ABSOLUTELY NO WARRANTY.");
  centeredText(
    "This is free software, and you are welcome to redistribute it under certain conditions.");
  centeredText("For details, see https://www.gnu.org/licenses/gpl-2.0.html.");

  ImGui::NewLine();

  centeredText("Find the full source code on GitHub:");
  centeredText("https://github.com/lethal-guitar/RigelEngine");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Open-source components & licenses");

  ImGui::NewLine();

  centeredText("RigelEngine incorporates some 3rd party open source code.");
  centeredText("The following libraries and components are used:");

  ImGui::NewLine();

  centeredTextBig("Simple DirectMedia Layer (SDL)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>");
  centeredText("https://www.libsdl.org");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("SDL Mixer", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>");
  centeredText("https://www.libsdl.org/projects/SDL_mixer");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Lyra", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright 2018-2019 René Ferdinand Rivera Morell");
  centeredText("Copyright 2017 Two Blue Cubes Ltd. All rights reserved.");
  centeredText("https://github.com/bfgroup/Lyra");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("DBOPL AdLib emulator (from DosBox)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2002-2021  The DOSBox Team");
  ImGui::NewLine();
  centeredText(
    "This program is free software; you can redistribute it and/or modify");
  centeredText(
    "it under the terms of the GNU General Public License as published by");
  centeredText(
    "the Free Software Foundation; either version 2 of the License, or");
  centeredText("(at your option) any later version.");
  ImGui::NewLine();
  centeredText(
    "This program is distributed in the hope that it will be useful,");
  centeredText(
    "but WITHOUT ANY WARRANTY; without even the implied warranty of");
  centeredText("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  centeredText("GNU General Public License for more details.");
  ImGui::NewLine();
  centeredText("https://www.gnu.org/licenses/gpl-2.0.html");
  ImGui::NewLine();
  centeredText("https://sourceforge.net/projects/dosbox");
  centeredText("https://github.com/dosbox-staging/dosbox-staging");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Nuked OPL3 AdLib emulator", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2013-2020 Nuke.YKT");
  ImGui::NewLine();
  centeredText(
    "Nuked OPL3 is free software: you can redistribute it and/or modify");
  centeredText(
    "it under the terms of the GNU Lesser General Public License as");
  centeredText("published by the Free Software Foundation, either version 2.1");
  centeredText("of the License, or (at your option) any later version.");
  ImGui::NewLine();
  centeredText("Nuked OPL3 is distributed in the hope that it will be useful,");
  centeredText(
    "but WITHOUT ANY WARRANTY; without even the implied warranty of");
  centeredText("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  centeredText("GNU Lesser General Public License for more details.");
  ImGui::NewLine();
  centeredText("https://www.gnu.org/licenses/lgpl-2.1.html");
  ImGui::NewLine();
  centeredText("https://github.com/nukeykt/Nuked-OPL3");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("EntityX", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2012 Alec Thomas");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/alecthomas/entityx");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("OpenGL/OpenGL ES loader code generated via glad", 1.4f);
  ImGui::Spacing();
  centeredText(
    "The glad code generator is Copyright (c) 2013-2021 David Herberth.");
  ImGui::Spacing();
  centeredText(
    "The generated code is in the public domain, except for khrplatform.h.");
  centeredText("The latter is under the following license:");
  ImGui::NewLine();
  centeredText("Copyright (c) 2008-2018 The Khronos Group Inc.");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a");
  centeredText(
    "copy of this software and/or associated documentation files (the");
  centeredText(
    "\"Materials\"), to deal in the Materials without restriction, including");
  centeredText(
    "without limitation the rights to use, copy, modify, merge, publish,");
  centeredText(
    "distribute, sublicense, and/or sell copies of the Materials, and to");
  centeredText(
    "permit persons to whom the Materials are furnished to do so, subject to");
  centeredText("the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included");
  centeredText("in all copies or substantial portions of the Materials.");
  ImGui::NewLine();
  centeredText(
    "THE MATERIALS ARE PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,");
  centeredText(
    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF");
  centeredText(
    "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.");
  centeredText(
    "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY");
  centeredText(
    "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,");
  centeredText(
    "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE");
  centeredText("MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.");
  ImGui::NewLine();
  centeredText("https://github.com/Dav1dde/glad");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("OpenGL Mathematics (GLM)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2005 - G-Truc Creation");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText("Restrictions:");
  centeredText(
    " By making use of the Software for military purposes, you choose to make a");
  centeredText(" Bunny unhappy.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/g-truc/glm");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Dear ImGui", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2014-2021 Omar Cornut");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/ocornut/imgui");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Dear ImGui file browser extension", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2019-2020 Zhuang Guan");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/AirGuanZ/imgui-filebrowser");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("JSON for Modern C++", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2013-2019 Niels Lohmann <http://nlohmann.me>.");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://json.nlohmann.me");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Resampler code from libspeex", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2007 Jean-Marc Valin");
  ImGui::NewLine();
  centeredText(
    "Redistribution and use in source and binary forms, with or without");
  centeredText(
    "modification, are permitted provided that the following conditions are");
  centeredText("met:");
  ImGui::NewLine();
  centeredText(
    "1. Redistributions of source code must retain the above copyright notice,");
  centeredText("this list of conditions and the following disclaimer.");
  ImGui::NewLine();
  centeredText(
    "2. Redistributions in binary form must reproduce the above copyright");
  centeredText(
    "notice, this list of conditions and the following disclaimer in the");
  centeredText(
    "documentation and/or other materials provided with the distribution.");
  ImGui::NewLine();
  centeredText(
    "3. The name of the author may not be used to endorse or promote products");
  centeredText(
    "derived from this software without specific prior written permission.");
  ImGui::NewLine();
  centeredText(
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR");
  centeredText(
    "IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES");
  centeredText("OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE");
  centeredText(
    "DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,");
  centeredText(
    "INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES");
  centeredText(
    "(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR");
  centeredText(
    "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)");
  centeredText(
    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,");
  centeredText(
    "STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN");
  centeredText(
    "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE");
  centeredText("POSSIBILITY OF SUCH DAMAGE.");
  ImGui::NewLine();
  centeredText("http://www.speex.org");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("static_vector", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright Gonzalo Brito Gadeschi 2015-2017");
  centeredText("Copyright Eric Niebler 2013-2014");
  centeredText("Copyright Casey Carter 2016");
  ImGui::NewLine();
  centeredText("https://github.com/gnzlbg/static_vector");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("stb_image & stb_rect_pack", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2017 Sean Barrett");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/nothings/stb");

  ImGui::NewLine();

  // Blank space at the end
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + creditsBoxHeight);

  // Measure the total size of the content on the first frame,
  // minus the blank space at the beginning. This is so that the credits
  // repeat quicker after the first full scroll through. If we would take
  // the full blank space into account, it would take a while for the start
  // to reappear after the end has scrolled off screen.
  if (mCreditsBoxContentHeightNormalized == 0.0f)
  {
    // We store the height divided by the window height, since the window might
    // be resized later. That would throw off our calculations if we were to
    // store the actual height. But this way, we simply multiply with the
    // actual window height at the time of drawing (see above).
    mCreditsBoxContentHeightNormalized =
      (ImGui::GetCursorPosY() - creditsBoxHeight) / windowSize.y;
  }

  ImGui::EndChild();
  ImGui::PopStyleColor();
}

} // namespace rigel::ui
