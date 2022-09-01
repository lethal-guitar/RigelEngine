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

#pragma once

#include <string>
#include <variant>

namespace rigel
{
struct IGameServiceProvider;
}
namespace rigel::ui
{
class MenuElementRenderer;
}


namespace rigel::ui
{

enum class MessagePriority
{
  Normal,
  HintMachineMessage,
  Menu
};


class IngameMessageDisplay
{
public:
  IngameMessageDisplay(
    IGameServiceProvider* pServiceProvider,
    MenuElementRenderer* pTextRenderer);

  void setMessage(
    std::string message,
    MessagePriority priority = MessagePriority::Normal);

  void update();
  void render();

private:
  // Multi-line messages are displayed line by line. The '*' character is used
  // as "line break" indicator. Each line is printed character by character,
  // then shown for the number of frames given by NEXT_LINE_DELAY. Afterwards,
  // either the next line starts printing, or the message disappears if already
  // finished.
  static constexpr auto NEXT_LINE_DELAY = 21;

  struct Idle
  {
  };

  struct Printing
  {
    Printing() = default;
    explicit Printing(const unsigned int offset)
      : mOffset(offset)
    {
    }

    unsigned int effectiveOffset() const { return mCharsPrinted + mOffset; }

    unsigned int mCharsPrinted = 0;
    unsigned int mOffset = 0;
  };

  struct Waiting
  {
    explicit Waiting(const unsigned int nextOffset)
      : mNextOffset(nextOffset)
    {
    }

    unsigned int mNextOffset = 0;
    unsigned int mFramesRemaining = NEXT_LINE_DELAY;
  };

  using State = std::variant<Idle, Printing, Waiting>;

  State mState;
  std::string mMessage;
  // TODO: Use a string_view instead of a second string once upgraded to C++ 17
  std::string mPrintedMessage;
  MessagePriority mCurrentPriority = MessagePriority::Normal;

  MenuElementRenderer* mpTextRenderer;
  IGameServiceProvider* mpServiceProvider;
};

} // namespace rigel::ui
