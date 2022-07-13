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
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "renderer/opengl.hpp"
#include "renderer/upscaling.hpp"
#include "sdl_utils/key_code.hpp"
#include "sdl_utils/platform.hpp"
#include "ui/credits.hpp"
#include "ui/hud_renderer.hpp"
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
  renderer::Renderer* pRenderer,
  const Type type)
  : mGamePathBrowser(
      ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_CloseOnEsc)
  , mpUserProfile(pUserProfile)
  , mpOptions(&pUserProfile->mOptions)
  , mpServiceProvider(pServiceProvider)
  , mpRenderer(pRenderer)
  , mEnableTopLevelMods(mpOptions->mEnableTopLevelMods)
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

    // Commit enabled mods on closing, so that the game is reloaded
    // when we close the options menu, and not every time we tick/untick
    // one of the checkboxes.
    if (moLocalModLibrary && mType == Type::Main)
    {
      mpOptions->mEnableTopLevelMods = mEnableTopLevelMods;
      moLocalModLibrary->replaceSelection(std::move(mModSelection));
      mpUserProfile->mModLibrary = std::move(*moLocalModLibrary);
    }
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

      withEnabledState(!mpOptions->mPerElementUpscalingEnabled, [&]() {
        auto upscalingFilterIndex =
          static_cast<int>(mpOptions->mUpscalingFilter);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20);
        ImGui::Combo(
          "Upscaling filter",
          &upscalingFilterIndex,
          "None (nearest neighbor)\0Sharp Bilinear\0Pixel-perfect (integer scaling)\0Bilinear\0");
        mpOptions->mUpscalingFilter =
          static_cast<data::UpscalingFilter>(upscalingFilterIndex);
      });

      if (mpOptions->mPerElementUpscalingEnabled)
      {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped(
          "NOTE: Upscaling options unavailable due to presence of high-res mods");
        ImGui::PopStyleColor();
      }
      else if (
        mpOptions->mUpscalingFilter == data::UpscalingFilter::PixelPerfect &&
        !renderer::canUsePixelPerfectScaling(mpRenderer, *mpOptions))
      {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped(
          "NOTE: Current screen resolution/window size is too small for pixel-perfect scaling (needs at least 1600x1200)!\nFalling back to sharp bilinear.");
        ImGui::PopStyleColor();
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

      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Test sound: ");

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

      const auto buttonSize =
        longestDescriptionSize + ImGui::GetStyle().FramePadding.x * 2;
      if (ImGui::Button(description.c_str(), {buttonSize, 0}))
      {
        mpServiceProvider->playSound(selectedSound.mId);
      }

      ImGui::SameLine();

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
      ImGui::TextUnformatted("Gamepad controls:");
      ImGui::NewLine();

      const auto extraFlags = isSmallScreen(windowSize)
        ? 0
        : ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX;

      if (ImGui::BeginTable(
            "gamepad_inputs",
            2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | extraFlags))
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Movement");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("D-pad or analog sticks");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Jump");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("A, B, LB, or LT");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Shoot");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("X, Y, RB, or RT");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Quick save");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("back/select + shoot (X, Y, RB, or RT)");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Quick load");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("back/select + jump (A, B, LB, or LT)");

        ImGui::EndTable();
      }

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

    if (ImGui::BeginTabItem("Modding"))
    {
      if (!moLocalModLibrary)
      {
        // We make a local copy of the mod library, to avoid triggering an
        // immediate restart in case our rescan causes changes to the mod
        // selection. The copy is committed back to the user profile when
        // closing the options menu.
        moLocalModLibrary = mpUserProfile->mModLibrary;

        if (mType == Type::Main)
        {
          moLocalModLibrary->rescan();
        }

        mModSelection = moLocalModLibrary->currentSelection();
      }

      ImGui::NewLine();

      if (mType != Type::Main)
      {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped("Mods can only be configured from the main menu");
        ImGui::PopStyleColor();
        ImGui::Spacing();
      }

      withEnabledState(mType == Type::Main, [&]() {
        ImGui::Checkbox(
          "Allow top-level mods (uncategorized files)", &mEnableTopLevelMods);
        ImGui::SameLine();

        if (ImGui::Button("Rescan"))
        {
          moLocalModLibrary->replaceSelection(mModSelection);
          moLocalModLibrary->rescan();
          mModSelection = moLocalModLibrary->currentSelection();
        }

        ImGui::NewLine();
      });

      ImGui::NewLine();

      if (mModSelection.empty())
      {
        ImGui::Text("No mods found");
      }
      else
      {
        std::optional<int> oIndexToMoveUp;
        std::optional<int> oIndexToMoveDown;

        const auto modListHeight = ImGui::GetContentRegionAvail().y -
          (shouldDrawGamePathChooser()
             ? mGamePathChooserHeightNormalized * ImGui::GetWindowSize().y
             : 0.0f);

        if (ImGui::BeginTable(
              "mod_list",
              3,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY,
              {-1, modListHeight}))
        {
          auto index = 0;
          for (auto& modStatus : mModSelection)
          {
            ImGui::TableNextRow();
            ImGui::PushID(index);

            // Column: Checkbox
            ImGui::TableNextColumn();
            withEnabledState(mType == Type::Main, [&]() {
              ImGui::Checkbox("", &modStatus.mIsEnabled);
            });

            // Column: Reordering buttons
            ImGui::TableNextColumn();

            withEnabledState(
              mType == Type::Main && mModSelection.size() > 1, [&]() {
                if (ImGui::Button("^"))
                {
                  oIndexToMoveUp = index;
                }

                ImGui::SameLine();

                if (ImGui::Button("v"))
                {
                  oIndexToMoveDown = index;
                }
              });

            // Column: Dir name
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(
              moLocalModLibrary->modDirName(modStatus.mIndex).c_str());

            ImGui::PopID();

            ++index;
          }

          ImGui::EndTable();
        }

        if (oIndexToMoveUp)
        {
          if (*oIndexToMoveUp > 0)
          {
            std::swap(
              mModSelection[*oIndexToMoveUp],
              mModSelection[*oIndexToMoveUp - 1]);
          }
          else
          {
            std::rotate(
              mModSelection.begin(),
              mModSelection.begin() + 1,
              mModSelection.end());
          }
        }

        if (oIndexToMoveDown)
        {
          const auto maxIndex = int(mModSelection.size() - 1);
          if (*oIndexToMoveDown < maxIndex)
          {
            std::swap(
              mModSelection[*oIndexToMoveDown],
              mModSelection[*oIndexToMoveDown + 1]);
          }
          else
          {
            std::rotate(
              mModSelection.begin(),
              mModSelection.end() - 1,
              mModSelection.end());
          }
        }
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Enhancements"))
    {
      ImGui::NewLine();

      if (!canUseHudStyle(mpOptions->mWidescreenHudStyle, mpRenderer))
      {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped(
          "NOTE: Current screen resolution/window size is too small for chosen HUD style, falling back to Classic.");
        ImGui::PopStyleColor();
      }

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

      ImGui::SameLine();

      withEnabledState(mpOptions->mWidescreenModeOn, [this]() {
        auto hudStyleIndex = static_cast<int>(mpOptions->mWidescreenHudStyle);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 18);
        ImGui::Combo(
          "HUD style",
          &hudStyleIndex,
          "Classic\0Remixed 1 (bottom + overlays)\0Remixed 2 (bottom only)\0");
        mpOptions->mWidescreenHudStyle =
          static_cast<data::WidescreenHudStyle>(hudStyleIndex);
      });

      withEnabledState(
        mpOptions->mWidescreenModeOn &&
          mpOptions->mWidescreenHudStyle == data::WidescreenHudStyle::Modern,
        [this]() {
          auto inverted = !mpOptions->mShowRadarInModernHud;
          ImGui::SameLine();
          ImGui::Checkbox("Hide radar", &inverted);
          mpOptions->mShowRadarInModernHud = !inverted;
        });

      ImGui::Checkbox("Quick saving", &mpOptions->mQuickSavingEnabled);
      ImGui::Checkbox("Skip intro sequence", &mpOptions->mSkipIntro);
      ImGui::Checkbox(
        "Smooth scrolling & movement", &mpOptions->mMotionSmoothing);
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
        renderer::OPENGL_VARIANT_NAME);

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

  drawCredits(creditsBoxWidth);

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
