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

#include "common/game_mode.hpp"
#include "renderer/texture.hpp"


namespace rigel {

/** Shows anti-piracy screen for registered version
 *
 * Shows the anti-piracy screen (LCR.MNI) until any key is pressed, then
 * switches to Intro/Demo loop.
 */
class AntiPiracyScreenMode : public GameMode {
public:
  explicit AntiPiracyScreenMode(Context context);

  std::unique_ptr<GameMode> updateAndRender(
    engine::TimeDelta,
    const std::vector<SDL_Event>& events
  ) override;

private:
  Context mContext;
  renderer::OwningTexture mTexture;
};

}
