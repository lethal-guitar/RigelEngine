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

#include "text_entry_widget.hpp"

#include "base/color.hpp"
#include "ui/menu_element_renderer.hpp"


namespace rigel::ui {

namespace {

constexpr auto CURSOR_ANIM_DELAY = 5;
constexpr auto NUM_CURSOR_ANIM_STATES = 4;

constexpr auto TEXT_COLOR = base::Color{109, 109, 109, 255};


bool isAsciiChar(const char chValue) {
  const auto value = static_cast<unsigned char>(chValue);
  return value <= 127;
}

}


TextEntryWidget::TextEntryWidget(
  const ui::MenuElementRenderer* pUiRenderer,
  const int posX,
  const int posY,
  const int maxTextLength
)
  : mpUiRenderer(pUiRenderer)
  , mPosX(posX)
  , mPosY(posY)
  , mMaxTextLength(maxTextLength)
{
}


void TextEntryWidget::handleEvent(const SDL_Event& event) {
  if (
    event.type == SDL_KEYDOWN &&
    event.key.keysym.sym == SDLK_BACKSPACE &&
    !mText.empty()
  ) {
    mText.pop_back();
  }

  if (event.type == SDL_TEXTINPUT) {
    const auto newChar = event.text.text[0];
    if (!isAsciiChar(newChar)) {
      return;
    }

    if (mText.size() < static_cast<size_t>(mMaxTextLength)) {
      mText += newChar;
    }
  }
}


void TextEntryWidget::updateAndRender(const engine::TimeDelta dt) {
  mElapsedTime += dt;

  // Text
  {
    // TODO: Instead of drawing an empty string to clear, draw a black
    // rectangle
    const auto spaces =
      std::string(static_cast<size_t>(mMaxTextLength + 1), ' ');
    mpUiRenderer->drawBigText(mPosX, mPosY, spaces, TEXT_COLOR);

    mpUiRenderer->drawBigText(mPosX, mPosY, mText, TEXT_COLOR);
  }

  // Cursor
  {
    const auto cursorAnimTicks = static_cast<int>(
      engine::timeToSlowTicks(mElapsedTime) / CURSOR_ANIM_DELAY);
    const auto cursorAnimState = cursorAnimTicks % NUM_CURSOR_ANIM_STATES;
    const auto cursorOffset = static_cast<int>(mText.size());

    mpUiRenderer->drawTextEntryCursor(
      mPosX + cursorOffset, mPosY, cursorAnimState);
  }
}


std::string_view TextEntryWidget::text() const {
  return mText;
}

}
