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
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <string>


namespace rigel::ui
{

class MenuElementRenderer;

class TextEntryWidget
{
public:
  enum class Style
  {
    Regular,
    BigText
  };

  // Position is given in tiles
  TextEntryWidget(
    const ui::MenuElementRenderer* pUiRenderer,
    int posX,
    int posY,
    int maxTextLength,
    Style textStyle,
    std::string_view initialText = "");

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);

  std::string_view text() const;

private:
  std::string mText;
  engine::TimeDelta mElapsedTime = 0.0;
  const ui::MenuElementRenderer* mpUiRenderer;
  int mPosX;
  int mPosY;
  int mMaxTextLength;
  Style mTextStyle;
};

} // namespace rigel::ui
