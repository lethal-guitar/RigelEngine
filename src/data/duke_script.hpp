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

#pragma once

#include <base/warnings.hpp>

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 30
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30

RIGEL_DISABLE_WARNINGS
#include <boost/variant.hpp>
RIGEL_RESTORE_WARNINGS

#include <string>
#include <vector>


namespace rigel { namespace data { namespace script {

struct DisableMenuFunctionality {};
struct EnableTextOffset {};
struct EnableTimeOutToDemo {};
struct FadeIn {};
struct FadeOut {};
struct ScheduleFadeInBeforeNextWaitState {};
struct ShowKeyBindings {};
struct StopNewsReporterAnimation {};
struct WaitForUserInput {};


struct ShowFullScreenImage {
  template<typename FileNameT>
  explicit ShowFullScreenImage(FileNameT&& image)
    : image(std::forward<FileNameT>(image))
  {
  }

  std::string image;
};


struct DrawSprite {
  DrawSprite(int x, int y, int spriteId, int frameNumber)
    : x(x)
    , y(y)
    , spriteId(spriteId)
    , frameNumber(frameNumber)
  {
  }

  int x;
  int y;
  int spriteId;
  int frameNumber;
};


struct DrawText {
  template<typename TextT>
  DrawText(int x, int y, TextT&& text)
    : x(x)
    , y(y)
    , text(std::forward<TextT>(text))
  {
  }

  int x;
  int y;
  std::string text;
};


struct DrawBigText {
  template<typename TextT>
  DrawBigText(const int x, const int y, const int colorIndex, TextT&& text)
    : x(x)
    , y(y)
    , colorIndex(colorIndex)
    , text(std::forward<TextT>(text))
  {
  }

  int x;
  int y;
  int colorIndex;
  std::string text;
};


struct Delay {
  explicit Delay(const int amount)
    : amount(amount)
  {
  }

  int amount;
};


struct SetPalette {
  template<typename FileNameT>
  explicit SetPalette(FileNameT&& paletteFile)
    : paletteFile(std::forward<FileNameT>(paletteFile))
  {
  }

  std::string paletteFile;
};


struct AnimateNewsReporter {
  explicit AnimateNewsReporter(const int talkDuration)
    : talkDuration(talkDuration)
  {
  }

  int talkDuration;
};


struct ShowMessageBox {
  template<typename MessageLinesT>
  ShowMessageBox(
    const int y,
    const int width,
    const int height,
    MessageLinesT&& messageLines
  )
    : y(y)
    , width(width)
    , height(height)
    , messageLines(std::forward<MessageLinesT>(messageLines))
  {
  }

  int y;
  int width;
  int height;
  std::vector<std::string> messageLines;
};


struct ShowMenuSelectionIndicator {
  explicit ShowMenuSelectionIndicator(const int yPos)
    : yPos(yPos)
  {
  }

  int yPos;
};


struct ConfigurePersistentMenuSelection {
  explicit ConfigurePersistentMenuSelection(const int slot)
    : slot(slot)
  {
  }

  int slot;
};


struct SetupCheckBoxes {
  using CheckBoxID = char;

  struct CheckBoxDefinition {
    int yPos;
    CheckBoxID id;
  };

  template<typename DefinitionListT>
  SetupCheckBoxes(const int xPos, DefinitionListT&& boxDefinitions)
    : xPos(xPos)
    , boxDefinitions(std::forward<DefinitionListT>(boxDefinitions))
  {
  }

  int xPos;
  std::vector<CheckBoxDefinition> boxDefinitions;
};


struct ShowSaveSlots {
  explicit ShowSaveSlots(const int selectedSlot)
    : mSelectedSlot(selectedSlot)
  {
  }

  int mSelectedSlot;
};


struct PagesDefinition;

using Action = boost::variant<
  AnimateNewsReporter,
  boost::recursive_wrapper<PagesDefinition>,
  ConfigurePersistentMenuSelection,
  Delay,
  DisableMenuFunctionality,
  DrawBigText,
  DrawSprite,
  DrawText,
  EnableTextOffset,
  EnableTimeOutToDemo,
  FadeIn,
  FadeOut,
  ScheduleFadeInBeforeNextWaitState,
  SetPalette,
  SetupCheckBoxes,
  ShowFullScreenImage,
  ShowKeyBindings,
  ShowMenuSelectionIndicator,
  ShowMessageBox,
  ShowSaveSlots,
  StopNewsReporterAnimation,
  WaitForUserInput
>;


using Script = std::vector<Action>;

struct PagesDefinition {
  template<typename PagesT>
  explicit PagesDefinition(PagesT&& pages)
    : pages(std::forward<PagesT>(pages))
  {
  }

  std::vector<Script> pages;
};

}}}
