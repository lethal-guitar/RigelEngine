/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "data/tutorial_messages.hpp"

#include <string>
#include <utility>


namespace rigel { namespace events {

struct ScreenFlash {
  ScreenFlash() = default;
  explicit ScreenFlash(base::Color color)
    : mColor(color)
  {
  }

  base::Color mColor = base::Color{255, 255, 255, 255};
};


struct PlayerMessage {
  PlayerMessage() = default;
  explicit PlayerMessage(std::string text)
    : mText(std::move(text))
  {
  }

  std::string mText;
};


struct TutorialMessage {
  TutorialMessage() = default;
  explicit TutorialMessage(const data::TutorialMessageId id)
    : mId(id)
  {
  }

  data::TutorialMessageId mId;
};


struct CheckPointActivated {
  base::Vector mPosition;
};

}}
