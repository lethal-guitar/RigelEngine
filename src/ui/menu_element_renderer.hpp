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

#include "base/warnings.hpp"
#include "engine/tile_renderer.hpp"
#include "engine/timing.hpp"
#include "loader/palette.hpp"
#include "sdl_utils/texture.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <string>


namespace rigel { namespace loader {
  class ResourceLoader;
}}


namespace rigel { namespace ui {

class MenuElementRenderer {
public:
  MenuElementRenderer(
    SDL_Renderer* pRenderer,
    const loader::ResourceLoader& resources,
    const loader::Palette16& palette = loader::INGAME_PALETTE);

  // Stateless API
  // --------------------------------------------------------------------------
  void drawText(int x, int y, const std::string& text) const;
  void drawMultiLineText(int x, int y, const std::string& text) const;
  void drawBigText(int x, int y, int colorIndex, const std::string& text) const;
  void drawMessageBox(int x, int y, int width, int height) const;

  void drawCheckBox(int x, int y, bool isChecked) const;

  void drawBonusScreenText(int x, int y, const std::string& text) const;

  // Stateful API
  // --------------------------------------------------------------------------
  void showMenuSelectionIndicator(int y);
  void hideMenuSelectionIndicator();

  void updateAndRenderAnimatedElements(engine::TimeDelta timeDelta);

private:
  void drawSelectionIndicator(int y, int state) const;
  void drawTextEntryCursor(int x, int y, int state) const;

  void drawMessageBoxRow(
    int x,
    int y,
    int width,
    int leftIndex,
    int middleIndex,
    int rightIndex) const;

private:
  engine::TileRenderer mSpriteSheetRenderer;
  mutable engine::TileRenderer mBigTextRenderer;

  loader::Palette16 mPalette;

  boost::optional<int> mMenuSelectionIndicatorPosition;
  bool mPendingMenuIndicatorErase = false;
  engine::TimeDelta mElapsedTime = 0;
};

}}
