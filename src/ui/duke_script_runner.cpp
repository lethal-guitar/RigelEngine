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

#include "duke_script_runner.hpp"

#include "base/warnings.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/tile_renderer.hpp"
#include "engine/timing.hpp"
#include "loader/resource_loader.hpp"
#include "utils/container_tools.hpp"
#include "utils/math_tools.hpp"
#include "ui/utils.hpp"

RIGEL_DISABLE_WARNINGS
#include <atria/variant/match_boost.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace ui {

using engine::TileRenderer;

using ExecutionResultOptional =
    boost::optional<DukeScriptRunner::ExecutionResult>;


namespace {

const auto NUM_NEWS_REPORTER_STATES = 4;
const auto NEWS_REPORTER_ACTOR_ID = 297;

const auto KEY_BINDINGS_START_X = 26;
const auto KEY_BINDINGS_START_Y = 7;
const auto SAVE_SLOT_START_X = 14;
const auto SAVE_SLOT_START_Y = 6;
const auto SELECTED_COLOR_INDEX = 3;
const auto UNSELECTED_COLOR_INDEX = 2;
const auto MENU_FONT_HEIGHT = 2;


// TODO: Move this out of here once it's required in other places

/* The random number lookup table embedded in the original game's executable.
 * In most cases, the game uses this table to generate random numbers. It has
 * a wrapping index into the table which is incremented every time a random
 * number is queried, and the corresponding number is returned.
 *
 * In some cases, the game incorporates additional information into decision
 * making to increase randomness, but many things are entirely based on this
 * table-based scheme.
 */
const std::array<int, 256> RANDOM_NUMBER_TABLE{
  0, 8, 109, 220, 222, 241, 149, 107, 75, 248, 254, 140, 16, 66, 74, 21, 211,
  47, 80, 242, 154, 27, 205, 128, 161, 89, 77, 36, 95, 110, 85, 48, 212, 140,
  211, 249, 22, 79, 200, 50, 28, 188, 52, 140, 202, 120, 68, 145, 62, 70, 184,
  190, 91, 197, 152, 224, 149, 104, 25, 178, 252, 182, 202, 182, 141, 197, 4,
  81, 181, 242, 145, 42, 39, 227, 156, 198, 225, 193, 219, 93, 122, 175, 249,
  0, 175, 143, 70, 239, 46, 246, 163, 53, 163, 109, 168, 135, 2, 235, 25, 92,
  20, 145, 138, 77, 69, 166, 78, 176, 173, 212, 166, 113, 94, 161, 41, 50, 239,
  49, 111, 164, 70, 60, 2, 37, 171, 75, 136, 156, 11, 56, 42, 146, 138, 229,
  73, 146, 77, 61, 98, 196, 135, 106, 63, 197, 195, 86, 96, 203, 113, 101, 170,
  247, 181, 113, 80, 250, 108, 7, 255, 237, 129, 226, 79, 107, 112, 166, 103,
  241, 24, 223, 239, 120, 198, 58, 60, 82, 128, 3, 184, 66, 143, 224, 145, 224,
  81, 206, 163, 45, 63, 90, 168, 114, 59, 33, 159, 95, 28, 139, 123, 98, 125,
  196, 15, 70, 194, 253, 54, 14, 109, 226, 71, 17, 161, 93, 186, 87, 244, 138,
  20, 52, 123, 251, 26, 36, 17, 46, 52, 231, 232, 76, 31, 221, 84, 37, 216,
  165, 212, 106, 197, 242, 98, 43, 39, 175, 254, 145, 190, 84, 118, 222, 187,
  136, 120, 163, 236, 249
};

}


DukeScriptRunner::DukeScriptRunner(const GameMode::Context& context)
  : mpResourceBundle(context.mpResources)
  , mCurrentPalette(loader::INGAME_PALETTE)
  , mpRenderer(context.mpRenderer)
  , mpServices(context.mpServiceProvider)
  , mMenuElementRenderer(context.mpRenderer, *context.mpResources)
  , mProgramCounter(0u)
{
}


void DukeScriptRunner::executeScript(const data::script::Script& script) {
  if (mCurrentPersistentSelectionSlot) {
    assert(mPagerState);
    mPersistentMenuSelections[*mCurrentPersistentSelectionSlot]
      = mPagerState->mCurrentPageIndex;
    mCurrentPersistentSelectionSlot = boost::none;
  }

  mPagerState = boost::none;
  mCheckBoxStates = boost::none;
  mFadeInBeforeNextWaitStateScheduled = false;
  mDisableMenuFunctionalityForNextPagesDefinition = false;

  startExecution(script);
}


void DukeScriptRunner::startExecution(const data::script::Script& script) {
  mCurrentInstructions = script;
  mProgramCounter = 0u;
  mState = State::ExecutingScript;

  mMenuItemWasSelected = false;
  mMenuElementRenderer.hideMenuSelectionIndicator();
  stopNewsReporterAnimation();
}


bool DukeScriptRunner::hasFinishedExecution() const {
  return mState == State::FinishedExecution ||
    mState == State::ExecutionInterrupted;
}


ExecutionResultOptional DukeScriptRunner::result() const {
  if (hasFinishedExecution()) {
    const auto selectedPage =
      hasMenuPages() ?
      boost::optional<int>(mPagerState->mCurrentPageIndex)
      : boost::none;

    auto terminationType = ScriptTerminationType::RanToCompletion;
    if (mState == State::ExecutionInterrupted) {
      terminationType = ScriptTerminationType::AbortedByUser;
    }
    if (hasMenuPages() && mMenuItemWasSelected) {
      terminationType = ScriptTerminationType::MenuItemSelected;
    }

    return ExecutionResult{terminationType, selectedPage};
  } else {
    return boost::none;
  }
}


bool DukeScriptRunner::isInWaitState() const {
  return mState == State::AwaitingUserInput;
}


void DukeScriptRunner::clearWaitState() {
  mState = State::ExecutingScript;
  mDelayState = boost::none;
}


void DukeScriptRunner::handleEvent(const SDL_Event& event) {
  if (hasFinishedExecution()) {
    return;
  }

  if (event.type != SDL_KEYDOWN) {
    return;
  }

  // Escape always aborts
  if (event.key.keysym.sym == SDLK_ESCAPE) {
    mState = State::ExecutionInterrupted;
    return;
  }

  // Any key stops a wait state (Delay or WaitForInput)
  if (isInWaitState()) {
    clearWaitState();
  }

  // Arrow keys, Enter and Space are used for pager interaction
  if (hasMenuPages()) {
    auto& state = *mPagerState;

    switch (event.key.keysym.sym) {
      case SDLK_LEFT:
      case SDLK_UP:
        selectPreviousPage(state);
        break;

      case SDLK_RIGHT:
      case SDLK_DOWN:
        selectNextPage(state);
        break;

      case SDLK_RETURN:
      case SDLK_SPACE:
        if (mPagerState->mMode == PagingMode::Menu) {
          selectCurrentMenuItem(state);
        } else {
          selectNextPage(state);
        }
        break;

      default:
        if (mPagerState->mMode == PagingMode::Menu) {
          // Since we cleared the wait state previously, we have to go back
          // to the current page
          executeCurrentPageScript(state);
        } else {
          selectNextPage(state);
        }
        break;
    }
  }
}


void DukeScriptRunner::updateAndRender(engine::TimeDelta dt) {
  mMenuElementRenderer.updateAndRenderAnimatedElements(dt);

  if (mDelayState) {
    handleDelay(*mDelayState, dt);
  }

  if (mNewsReporterAnimationState) {
    animateNewsReporter(*mNewsReporterAnimationState, dt);
  }

  if (hasCheckBoxes()) {
    displayCheckBoxes(*mCheckBoxStates);
  }

  while (mState == State::ExecutingScript) {
    interpretNextAction();
  }

  if (mFadeInBeforeNextWaitStateScheduled && !hasFinishedExecution()) {
    mpServices->fadeInScreen();
    mFadeInBeforeNextWaitStateScheduled = false;
  }
}


void DukeScriptRunner::displayCheckBoxes(const CheckBoxesState& state) {
  const auto xPos = state.mPosX;

  for (const auto& box : state.mStates) {
    const auto yPos = box.mPosY;
    mMenuElementRenderer.drawCheckBox(xPos, yPos, box.mChecked);
  }
}


void DukeScriptRunner::handleDelay(
  DelayState& state,
  const engine::TimeDelta timeDelta
) {
  state.mElapsedTime += timeDelta;

  if (state.mElapsedTime >= engine::slowTicksToTime(state.mTicksToWait)) {
    clearWaitState();
  }
}


void DukeScriptRunner::animateNewsReporter(
  NewsReporterState& state,
  const engine::TimeDelta timeDelta
) {
  state.mElapsedTime += timeDelta;
  const auto elapsedTicks = engine::timeToFastTicks(state.mElapsedTime);

  const auto elapsedFrames = elapsedTicks / 25.0;
  const auto numFramesAlreadyTalked = static_cast<int>(elapsedFrames);

  if (numFramesAlreadyTalked < state.mTalkDuration) {
    const auto randomNumber =
      RANDOM_NUMBER_TABLE[numFramesAlreadyTalked % RANDOM_NUMBER_TABLE.size()];
    drawSprite(
      NEWS_REPORTER_ACTOR_ID,
      randomNumber % NUM_NEWS_REPORTER_STATES,
      0,
      0);
  } else {
    stopNewsReporterAnimation();
  }
}


void DukeScriptRunner::stopNewsReporterAnimation() {
  if (mNewsReporterAnimationState) {
    drawSprite(NEWS_REPORTER_ACTOR_ID, 0, 0, 0);
  }
  mNewsReporterAnimationState = boost::none;
}


void DukeScriptRunner::interpretNextAction() {
  using namespace data::script;

  if (mProgramCounter >= mCurrentInstructions.size()) {
    mState = State::FinishedExecution;
    return;
  }

  atria::variant::match(
    mCurrentInstructions[mProgramCounter++],

    [this](const AnimateNewsReporter& action) {
      mNewsReporterAnimationState = NewsReporterState{action.talkDuration};
    },

    [this](const FadeIn&) {
      mpServices->fadeInScreen();
    },

    [this](const FadeOut&) {
      mpServices->fadeOutScreen();
    },

    [this](const ShowMenuSelectionIndicator& action) {
      mMenuElementRenderer.showMenuSelectionIndicator(action.yPos);
      if (hasCheckBoxes()) {
        mCheckBoxStates->mCurrentMenuPosY = action.yPos;
      }
    },

    [this](const StopNewsReporterAnimation&) {
      stopNewsReporterAnimation();
    },

    [this](const ShowFullScreenImage& showImage) {
      updatePalette(
        mpResourceBundle->loadPaletteFromFullScreenImage(showImage.image));

      const auto imageTexture = fullScreenImageAsTexture(
        mpRenderer,
        *mpResourceBundle,
        showImage.image);
      imageTexture.render(mpRenderer, 0, 0);
    },

    [this](const Delay& delay) {
      mDelayState = DelayState{delay.amount};
      mState = State::AwaitingUserInput;
    },

    [this](const WaitForUserInput&) {
      mState = State::AwaitingUserInput;
    },

    [this](const DrawBigText& action) {
      mMenuElementRenderer.drawBigText(
        action.x + 2,
        action.y,
        action.colorIndex,
        action.text);
    },

    [this](const DrawText& action) {
      mMenuElementRenderer.drawText(action.x, action.y, action.text);
    },

    [this](const DrawSprite& action) {
      drawSprite(
        static_cast<data::ActorID>(action.spriteId),
        action.frameNumber,
        action.x,
        action.y);
    },

    [this](const SetPalette& action) {
      updatePalette(loader::load6bitPalette16(
        mpResourceBundle->mFilePackage.file(action.paletteFile)));
    },

    [this](const SetupCheckBoxes& action) {
      CheckBoxesState state;
      state.mPosX = action.xPos;
      state.mCurrentMenuPosY = 0;
      state.mStates = utils::transformed(action.boxDefinitions,
        [](const auto& definition) {
          return CheckBoxState{
            definition.yPos,
            false,
            definition.id
          };
        });

      mCheckBoxStates = state;
      displayCheckBoxes(state);
    },

    [this](const ShowMessageBox& messageBoxDefinition) {
      const auto xPos = (40 - messageBoxDefinition.width) / 2;
      mMenuElementRenderer.drawMessageBox(
        xPos,
        messageBoxDefinition.y,
        messageBoxDefinition.width,
        messageBoxDefinition.height);

      auto yPos = messageBoxDefinition.y + 1;
      const auto availableWidth = messageBoxDefinition.width - 1;
      for (const auto& line : messageBoxDefinition.messageLines) {
        const auto lineLength = static_cast<int>(line.size());
        const auto offsetToCenter = (availableWidth - lineLength) / 2;

        mMenuElementRenderer.drawText(xPos + 1 + offsetToCenter, yPos, line);
        ++yPos;
      }
    },

    [this](const ScheduleFadeInBeforeNextWaitState&) {
      mFadeInBeforeNextWaitStateScheduled = true;
    },

    [this](const ConfigurePersistentMenuSelection& action) {
      const auto slotIndex = action.slot;
      const auto iter = mPersistentMenuSelections.find(slotIndex);
      if (iter == mPersistentMenuSelections.end()) {
        mPersistentMenuSelections.emplace(0, 0);
      }

      mCurrentPersistentSelectionSlot = slotIndex;
    },

    [this](const DisableMenuFunctionality&) {
      if (mPagerState) {
        mPagerState->mMode = PagingMode::PagingOnly;
      } else {
        mDisableMenuFunctionalityForNextPagesDefinition = true;
      }
    },

    [this](const PagesDefinition& definition) {
      mPagerState = PagerState{
        definition.pages,
        PagingMode::Menu,
        0,
        static_cast<int>(definition.pages.size() - 1)
      };

      if (mCurrentPersistentSelectionSlot) {
        mPagerState->mCurrentPageIndex =
          mPersistentMenuSelections[*mCurrentPersistentSelectionSlot];
      }

      if (mDisableMenuFunctionalityForNextPagesDefinition) {
        mPagerState->mMode = PagingMode::PagingOnly;
        mDisableMenuFunctionalityForNextPagesDefinition = false;
      }

      executeCurrentPageScript(*mPagerState);
    },

    [this](const EnableTextOffset&) {
      // TODO
    },

    [this](const EnableTimeOutToDemo&) {
      // TODO
    },

    [this](const ShowKeyBindings&) {
      drawCurrentKeyBindings();
    },

    [this](const ShowSaveSlots& action) {
      drawSaveSlotNames(action.mSelectedSlot);
    }
  );
}


void DukeScriptRunner::drawSprite(
  const data::ActorID id,
  const int frame,
  const int x,
  const int y
) {
  const auto actorData =
    mpResourceBundle->mActorImagePackage.loadActor(id, mCurrentPalette);
  const auto& frameData = actorData.mFrames.at(frame);
  const auto& image = frameData.mFrameImage;

  const auto spriteHeightTiles =
    data::pixelsToTiles(
      static_cast<int>(image.height()));
  const auto pos = base::Vector{x - 1, y};
  const auto topLeft = pos - base::Vector(0, spriteHeightTiles - 1);

  const auto topLeftPx = data::tileVectorToPixelVector(topLeft);
  const auto drawOffsetPx =
    data::tileVectorToPixelVector(frameData.mDrawOffset);

  sdl_utils::OwningTexture spriteTexture(mpRenderer, image);
  spriteTexture.render(mpRenderer, topLeftPx + drawOffsetPx);
}


void DukeScriptRunner::selectNextPage(PagerState& state) {
  ++state.mCurrentPageIndex;
  if (state.mCurrentPageIndex > state.mMaxPageIndex) {
    state.mCurrentPageIndex = 0;
  }
  executeCurrentPageScript(state);

  if (mPagerState->mMode == PagingMode::Menu) {
    mpServices->playSound(data::SoundId::MenuSelect);
  }
}


void DukeScriptRunner::selectPreviousPage(PagerState& state) {
  --state.mCurrentPageIndex;
  if (state.mCurrentPageIndex < 0) {
    state.mCurrentPageIndex = state.mMaxPageIndex;
  }
  executeCurrentPageScript(state);

  if (mPagerState->mMode == PagingMode::Menu) {
    mpServices->playSound(data::SoundId::MenuSelect);
  }
}


void DukeScriptRunner::executeCurrentPageScript(PagerState& state) {
  startExecution(state.mPageScripts[state.mCurrentPageIndex]);
}


void DukeScriptRunner::selectCurrentMenuItem(PagerState& pagerState) {
  if (hasCheckBoxes()) {
    auto& checkBoxStates = *mCheckBoxStates;
    const auto currentMenuPosY = checkBoxStates.mCurrentMenuPosY;
    const auto currentCheckBoxIter = find_if(
      checkBoxStates.mStates.begin(),
      checkBoxStates.mStates.end(),
      [currentMenuPosY](const auto& state) {
        return state.mPosY == currentMenuPosY;
      });

    if (currentCheckBoxIter != checkBoxStates.mStates.end()) {
      currentCheckBoxIter->mChecked = !currentCheckBoxIter->mChecked;
      executeCurrentPageScript(pagerState);

      mpServices->playSound(data::SoundId::MenuToggle);
      return;
    }
  }

  mMenuItemWasSelected = true;
}

void DukeScriptRunner::drawSaveSlotNames(const int selectedIndex) {
  for (auto i = 0; i < 8; ++i) {
    mMenuElementRenderer.drawBigText(
      SAVE_SLOT_START_X,
      SAVE_SLOT_START_Y + i*MENU_FONT_HEIGHT,
      i == selectedIndex ? SELECTED_COLOR_INDEX : UNSELECTED_COLOR_INDEX,
      "Empty");
  }
}


void DukeScriptRunner::drawCurrentKeyBindings() {
  static std::array<std::string, 6> keyNames{
    "ALT",
    "CTRL",
    "Up",
    "Down",
    "Left",
    "Right"
  };

  for (auto i = 0u; i < keyNames.size(); ++i) {
    mMenuElementRenderer.drawText(
      KEY_BINDINGS_START_X,
      KEY_BINDINGS_START_Y + i*MENU_FONT_HEIGHT,
      keyNames[i]);
  }
}


void DukeScriptRunner::updatePalette(const loader::Palette16& palette) {
  // TODO think about optimizing this maybe, to only update if actually
  // changed, e.g. if (palette != mCurrentPalette) or maybe have a
  // 'paletteSource' string (name of palette file or fullscreen image file),
  // which can be compared to determine if update needed.

  mCurrentPalette = palette;
  mMenuElementRenderer =
    MenuElementRenderer(mpRenderer, *mpResourceBundle, palette);
}


bool DukeScriptRunner::hasMenuPages() const {
  return static_cast<bool>(mPagerState);
}


bool DukeScriptRunner::hasCheckBoxes() const {
  return static_cast<bool>(mCheckBoxStates);
}

}}
