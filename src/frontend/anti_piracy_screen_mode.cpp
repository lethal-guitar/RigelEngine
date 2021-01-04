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

#include "anti_piracy_screen_mode.hpp"

#include "loader/resource_loader.hpp"

#include "intro_demo_loop_mode.hpp"

#include "ui/menu_navigation.hpp"


namespace rigel {

AntiPiracyScreenMode::AntiPiracyScreenMode(
  Context context,
  const bool isFirstLaunch)
  : mContext(context)
  , mTexture(context.mpRenderer, context.mpResources->loadAntiPiracyImage())
  , mIsFirstLaunch(isFirstLaunch)
{
}


std::unique_ptr<GameMode> AntiPiracyScreenMode::updateAndRender(
  engine::TimeDelta,
  const std::vector<SDL_Event>& events)
{
  using std::any_of;
  using std::begin;
  using std::end;

  mTexture.render(0, 0);

  const auto anyButtonPressed = any_of(begin(events), end(events),
    [](const SDL_Event& event) {
      return ui::isButtonPress(event);
    });

  if (anyButtonPressed) {
    return std::make_unique<IntroDemoLoopMode>(
      mContext,
      mIsFirstLaunch
        ? IntroDemoLoopMode::Type::AtFirstLaunch
        : IntroDemoLoopMode::Type::DuringGameStart);
  }

  return {};
}

}
