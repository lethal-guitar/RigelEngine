/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "ingame_message_display.hpp"

#include "base/match.hpp"
#include "data/sound_ids.hpp"
#include "frontend/game_service_provider.hpp"
#include "ui/menu_element_renderer.hpp"

#include <cctype>


namespace rigel::ui
{

namespace
{

constexpr auto NEXT_LINE_MARKER = '*';
constexpr auto CHARS_PER_LINE = 37;

} // namespace

IngameMessageDisplay::IngameMessageDisplay(
  IGameServiceProvider* pServiceProvider,
  MenuElementRenderer* pTextRenderer)
  : mpTextRenderer(pTextRenderer)
  , mpServiceProvider(pServiceProvider)
{
}


void IngameMessageDisplay::setMessage(
  std::string message,
  MessagePriority priority)
{
  if (
    !message.empty() &&
    (priority >= mCurrentPriority || !std::holds_alternative<Printing>(mState)))
  {
    mMessage = std::move(message);
    mCurrentPriority = priority;
    mPrintedMessage.clear();
    mState = Printing{};
  }
}


void IngameMessageDisplay::update()
{
  base::match(
    mState,
    [](const Idle&) {},

    [this](Printing& state) {
      const auto nextChar = mMessage[state.effectiveOffset()];

      const auto foundNextLineMarker = nextChar == NEXT_LINE_MARKER;
      if (!foundNextLineMarker)
      {
        mPrintedMessage.push_back(static_cast<char>(std::toupper(nextChar)));
        if (nextChar != ' ')
        {
          mpServiceProvider->playSound(data::SoundId::IngameMessageTyping);
        }
      }

      ++state.mCharsPrinted;

      const auto messageConsumed = state.effectiveOffset() >= mMessage.size();
      // clang-format off
      const auto endOfLine =
        state.mCharsPrinted == CHARS_PER_LINE ||
        foundNextLineMarker ||
        messageConsumed;
      // clang-format on

      if (endOfLine)
      {
        mState = Waiting{state.effectiveOffset()};
      }
    },

    [this](Waiting& state) {
      --state.mFramesRemaining;
      if (state.mFramesRemaining == 0)
      {
        mPrintedMessage.clear();

        if (state.mNextOffset < mMessage.size())
        {
          mState = Printing{state.mNextOffset};
        }
        else
        {
          mState = Idle{};
        }
      }
    });
}


void IngameMessageDisplay::render()
{
  if (!mPrintedMessage.empty())
  {
    mpTextRenderer->drawSmallWhiteText(0, 0, mPrintedMessage);
  }
}

} // namespace rigel::ui
