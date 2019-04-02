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

#include "base/match.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/tile_renderer.hpp"
#include "engine/timing.hpp"
#include "loader/resource_loader.hpp"
#include "utils/container_tools.hpp"
#include "ui/utils.hpp"

#include "game_service_provider.hpp"


namespace rigel { namespace ui {

using engine::TileRenderer;

using ExecutionResultOptional =
    std::optional<DukeScriptRunner::ExecutionResult>;


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


const auto SKILL_LEVEL_SLOT = 0;
const auto GAME_SPEED_SLOT = 8;
const auto INITIAL_SKILL_SELECTION = 1;
const auto INITIAL_GAME_SPEED = 3;

}


DukeScriptRunner::DukeScriptRunner(
  loader::ResourceLoader* pResourceLoader,
  engine::Renderer* pRenderer,
  IGameServiceProvider* pServiceProvider
)
  : mpResourceBundle(pResourceLoader)
  , mCurrentPalette(loader::INGAME_PALETTE)
  , mpRenderer(pRenderer)
  , mpServices(pServiceProvider)
  , mUiSpriteSheetRenderer(
     engine::OwningTexture{
        pRenderer, pResourceLoader->loadTiledFullscreenImage("STATUS.MNI")},
      pRenderer)
  , mMenuElementRenderer(&mUiSpriteSheetRenderer, pRenderer, *pResourceLoader)
  , mProgramCounter(0u)
{
  // Default menu pre-selections at game start
  mPersistentMenuSelections.emplace(SKILL_LEVEL_SLOT, INITIAL_SKILL_SELECTION);
  mPersistentMenuSelections.emplace(GAME_SPEED_SLOT, INITIAL_GAME_SPEED);
}


void DukeScriptRunner::executeScript(const data::script::Script& script) {
  mCurrentPersistentSelectionSlot = std::nullopt;
  mPagerState = std::nullopt;
  mCheckBoxStates = std::nullopt;
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
      std::optional<int>(mPagerState->mCurrentPageIndex)
      : std::nullopt;

    auto terminationType = ScriptTerminationType::RanToCompletion;
    if (mState == State::ExecutionInterrupted) {
      terminationType = ScriptTerminationType::AbortedByUser;
    }
    if (hasMenuPages() && mMenuItemWasSelected) {
      terminationType = ScriptTerminationType::MenuItemSelected;
    }

    return ExecutionResult{terminationType, selectedPage};
  } else {
    return std::nullopt;
  }
}


bool DukeScriptRunner::isInWaitState() const {
  return mState == State::AwaitingUserInput;
}


void DukeScriptRunner::clearWaitState() {
  mState = State::ExecutingScript;
  mDelayState = std::nullopt;
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
      engine::RANDOM_NUMBER_TABLE[
        numFramesAlreadyTalked % engine::RANDOM_NUMBER_TABLE.size()];
    const auto newTalkFrame = randomNumber % NUM_NEWS_REPORTER_STATES;

    if (newTalkFrame != state.mLastTalkFrame) {
      drawSprite(
        NEWS_REPORTER_ACTOR_ID,
        newTalkFrame,
        0,
        0);
      state.mLastTalkFrame = newTalkFrame;
    }
  } else {
    stopNewsReporterAnimation();
  }
}


void DukeScriptRunner::stopNewsReporterAnimation() {
  if (mNewsReporterAnimationState) {
    drawSprite(NEWS_REPORTER_ACTOR_ID, 0, 0, 0);
  }
  mNewsReporterAnimationState = std::nullopt;
}


void DukeScriptRunner::drawBigText(
  const int x,
  const int y,
  const int colorIndex,
  const std::string& text
) const {
  mpRenderer->setColorModulation(mCurrentPalette.at(colorIndex));
  mMenuElementRenderer.drawBigText(x, y, text);
  mpRenderer->setColorModulation(base::Color{255, 255, 255, 255});
}


void DukeScriptRunner::interpretNextAction() {
  using namespace data::script;

  if (mProgramCounter >= mCurrentInstructions.size()) {
    mState = State::FinishedExecution;
    return;
  }

  base::match(
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
      mpRenderer->submitBatch();
    },

    [this](const Delay& delay) {
      mDelayState = DelayState{delay.amount};
      mState = State::AwaitingUserInput;
    },

    [this](const WaitForUserInput&) {
      mState = State::AwaitingUserInput;
    },

    [this](const DrawBigText& action) {
      drawBigText(
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
      mCurrentPersistentSelectionSlot = action.slot;
    },

    [this](const DisableMenuFunctionality&) {
      if (mPagerState) {
        mPagerState->mMode = PagingMode::PagingOnly;
      } else {
        mDisableMenuFunctionalityForNextPagesDefinition = true;
      }
    },

    [this](const std::shared_ptr<PagesDefinition>& pDefinition) {
      const auto& definition = *pDefinition;

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

  engine::OwningTexture spriteTexture(mpRenderer, image);
  spriteTexture.render(mpRenderer, topLeftPx + drawOffsetPx);
  mpRenderer->submitBatch();
}


void DukeScriptRunner::selectNextPage(PagerState& state) {
  ++state.mCurrentPageIndex;
  if (state.mCurrentPageIndex > state.mMaxPageIndex) {
    state.mCurrentPageIndex = 0;
  }

  onPageChanged(state);
}


void DukeScriptRunner::selectPreviousPage(PagerState& state) {
  --state.mCurrentPageIndex;
  if (state.mCurrentPageIndex < 0) {
    state.mCurrentPageIndex = state.mMaxPageIndex;
  }

  onPageChanged(state);
}


void DukeScriptRunner::onPageChanged(PagerState& state) {
  executeCurrentPageScript(state);

  if (mPagerState->mMode == PagingMode::Menu) {
    mpServices->playSound(data::SoundId::MenuSelect);
  }

  if (mCurrentPersistentSelectionSlot) {
    mPersistentMenuSelections[*mCurrentPersistentSelectionSlot] =
      state.mCurrentPageIndex;
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
    drawBigText(
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
  mUiSpriteSheetRenderer = engine::TileRenderer{
     engine::OwningTexture{
        mpRenderer,
        mpResourceBundle->loadTiledFullscreenImage(
          "STATUS.MNI", mCurrentPalette)},
      mpRenderer};
}


bool DukeScriptRunner::hasMenuPages() const {
  return static_cast<bool>(mPagerState);
}


bool DukeScriptRunner::hasCheckBoxes() const {
  return static_cast<bool>(mCheckBoxStates);
}

}}
