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

#include "base/boost_variant.hpp"

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
  explicit ShowFullScreenImage(FileNameT&& image_)
    : image(std::forward<FileNameT>(image_))
  {
  }

  std::string image;
};


struct DrawSprite {
  DrawSprite(int x_, int y_, int spriteId_, int frameNumber_)
    : x(x_)
    , y(y_)
    , spriteId(spriteId_)
    , frameNumber(frameNumber_)
  {
  }

  int x;
  int y;
  int spriteId;
  int frameNumber;
};


struct DrawText {
  template<typename TextT>
  DrawText(int x_, int y_, TextT&& text_)
    : x(x_)
    , y(y_)
    , text(std::forward<TextT>(text_))
  {
  }

  int x;
  int y;
  std::string text;
};


struct DrawBigText {
  template<typename TextT>
  DrawBigText(const int x_, const int y_, const int colorIndex_, TextT&& text_)
    : x(x_)
    , y(y_)
    , colorIndex(colorIndex_)
    , text(std::forward<TextT>(text_))
  {
  }

  int x;
  int y;
  int colorIndex;
  std::string text;
};


struct Delay {
  explicit Delay(const int amount_)
    : amount(amount_)
  {
  }

  int amount;
};


struct SetPalette {
  template<typename FileNameT>
  explicit SetPalette(FileNameT&& paletteFile_)
    : paletteFile(std::forward<FileNameT>(paletteFile_))
  {
  }

  std::string paletteFile;
};


struct AnimateNewsReporter {
  explicit AnimateNewsReporter(const int talkDuration_)
    : talkDuration(talkDuration_)
  {
  }

  int talkDuration;
};


struct ShowMessageBox {
  template<typename MessageLinesT>
  ShowMessageBox(
    const int y_,
    const int width_,
    const int height_,
    MessageLinesT&& messageLines_
  )
    : y(y_)
    , width(width_)
    , height(height_)
    , messageLines(std::forward<MessageLinesT>(messageLines_))
  {
  }

  int y;
  int width;
  int height;
  std::vector<std::string> messageLines;
};


struct ShowMenuSelectionIndicator {
  explicit ShowMenuSelectionIndicator(const int yPos_)
    : yPos(yPos_)
  {
  }

  int yPos;
};


struct ConfigurePersistentMenuSelection {
  explicit ConfigurePersistentMenuSelection(const int slot_)
    : slot(slot_)
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
  SetupCheckBoxes(const int xPos_, DefinitionListT&& boxDefinitions_)
    : xPos(xPos_)
    , boxDefinitions(std::forward<DefinitionListT>(boxDefinitions_))
  {
  }

  int xPos;
  std::vector<CheckBoxDefinition> boxDefinitions;
};


struct ShowSaveSlots {
  explicit ShowSaveSlots(const int selectedSlot_)
    : mSelectedSlot(selectedSlot_)
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
  explicit PagesDefinition(PagesT&& pages_)
    : pages(std::forward<PagesT>(pages_))
  {
  }

  std::vector<Script> pages;
};

}}}
