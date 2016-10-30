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

#include "menu_element_renderer.hpp"

#include <data/game_traits.hpp>
#include <data/unit_conversions.hpp>
#include <engine/timing.hpp>
#include <loader/resource_loader.hpp>

#include <cassert>
#include <cmath>
#include <stdexcept>


/* FONT FINDINGS
 *
 * Font actor: 29 (2-planar, 1st mask, 2nd grayscale)
 *
 * |   0 | A |
 * | ...     |
 * |  25 | Z |
 * |  26 | 0 |
 * |  27 | 1 |
 * |  ...    |
 * |  35 | 9 |
 * |  36 | ? |
 * |  37 | , |
 * |  38 | . |
 * |  39 | ! |
 * |  40 | <big block> |
 * |  41 | a |
 * |  42 | b |
 * |  ...    |
 * |  66 | z |
 *
 * STATUS.MNI-based fonts:
 *
 * Multiple fonts here:
 *   1. Orange, small, nearly complete ASCII. 1 char == 1 tile. Used in Menus
 *        col  0, row 21: ASCII chars  22 -  61
 *        col  0, row 22: ASCII chars  62 - 90, 97-107
 *        col 17, row 23: ASCII chars 108 - 122
 *
 *   2. Big, numbers (green) and letters (white). 1 char == 4 tiles. Used for
 *      bonus screen
 *        col 0, row 0: ASCII chars 48-57, 65-74
 *        col 0, row 2: ASCII chars 75-90,37,61,46,33
 *
 *   3. Small, bold, white, letters and some punctuation. 1 char == 1 tile.
 *      Used for in-game messages
 *        col 20, row  6: ASCII chars 48-84
 *        col 17, row 24: ASCII chars 85-90,44,46,33,63
 *
 *   4. Blue, gray background. Numbers only (see hud_renderer.cpp). Used for
 *      score and level number display.
 *
 *
 ******************************************************************************
 *
 * Other STATUS.MNI stuff:
 *
 * 1. Rotating arrow for menu selection
 *     8 images, 2x2 tiles. Starts at col 0, row 9
 *
 * 2. Toggle box for menu (options)
 *
 *     2 images, 2x2 tiles.
 *       - col 20, row 7: Unchecked
 *       - col 22, row 7: Checked
 *
 * 3. Message box borders. Each 1 tile big, all in row 4
 *     | col | function     |
 *     |   0 | top-left     |
 *     |   1 | top          |
 *     |   2 | top-right    |
 *     |   3 | right        |
 *     |   4 | bottom-right |
 *     |   5 | bottom       |
 *     |   6 | bottom-left  |
 *     |   7 | left         |
 *
 * 4. Blinking cursor, for save name/hi-score entry
 *
 *     4 images, 1x1 tile. Starts at col 9, row 4
 *
 *
 *
 ******************************************************************************
 *
 * MsgBox slide in animation:
 *
 *
 * void enterMsgBox(int yPos, int width, int height) {
 *   auto xPos = (40 - width) / 2;
 *   auto centeredY = yPos + height/2;
 *
 *   int animatedWidth = 1;
 *   for (int i=19; i > xPos; --i) {
 *     animatedWidth += 2;
 *     // x, y, width, height
 *     drawMsgBox(i, centeredY, animatedWidth, 2);
 *     // Wait one 140 Hz tick
 *   }
 *
 *   int targetPosY = yPos + (height % 2 == 0 ? 1 : 0);
 *   int animatedHeight = 0;
 *   for (int i=centeredY; targetPosY < i; --i) {
 *     animatedHeight += 2;
 *     drawMsgBox(xPos, i, width, animatedHeight);
 *     // Wait one 140 Hz tick
 *   }
 * }
 *
 */



namespace rigel { namespace ui {

namespace {

const auto NUM_MENU_INDICATOR_STATES = 8;
const auto MENU_INDICATOR_STATE_FOR_CLEARING = NUM_MENU_INDICATOR_STATES + 1;

}


MenuElementRenderer::MenuElementRenderer(
  SDL_Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const loader::Palette16& palette
)
  : mpRenderer(pRenderer)
  , mSpriteSheetRenderer(
    sdl_utils::OwningTexture(
      pRenderer,
      resources.loadTiledFullscreenImage("STATUS.MNI", palette)),
    pRenderer)
  , mPalette(palette)
{
  const auto font = resources.mActorImagePackage.loadFont();
  if (font.mCharacterCount != 67) {
    throw std::runtime_error("Font data incomplete");
  }

  mFontBitmapSize = font.mSingleCharacterBitmapSize;
  mFontTexture = sdl_utils::OwningTexture(pRenderer, font.mCharacterBitmaps);
}


void MenuElementRenderer::drawText(
  int x,
  int y,
  const std::string& text
) const {
  for (auto i=0u; i<text.size(); ++i) {
    const auto ch = static_cast<uint8_t>(text[i]);

    int spriteSheetIndex = 0;
    if (ch < 62) {
      spriteSheetIndex = 21*40 + (ch - 22);
    } else if (ch <= 90) {
      spriteSheetIndex = 22*40 + (ch - 62);
    } else if (ch >= 97 && ch < 108) {
      spriteSheetIndex = 22*40 + (ch - 68);
    } else if (ch >= 108 && ch <= 122) {
      spriteSheetIndex = 23*40 + 17 + (ch - 108);
    } else {
      // Skip non-renderable chars
      continue;
    }

    mSpriteSheetRenderer.renderTile(spriteSheetIndex, x + i, y);
  }
}


void MenuElementRenderer::drawBigText(
  int x,
  int y,
  int colorIndex,
  const std::string& text
) const {
  const auto& color = mPalette.at(colorIndex);
  mFontTexture.setColorMod(color.r, color.g, color.b);

  for (auto i=0u; i<text.size(); ++i) {
    const auto ch = static_cast<uint8_t>(text[i]);

    int index = 0;
    if (ch >= 65 && ch <= 90) {
      index = ch - 65;
    } else if (ch >= 48 && ch <= 57) {
      index = ch - 48 + 26;
    } else if (ch >= 97 && ch <= 122) {
      index = ch - 97 + 41;
    } else if (ch == 63) {
      index = 36;
    } else if (ch == 44) {
      index = 37;
    } else if (ch == 46) {
      index = 38;
    } else if (ch == 33) {
      index = 39;
    } else {
      index = 40;
    }

    const auto characterPosition = static_cast<int>(i);
    const auto pixelPosX = data::tilesToPixels(x) +
      characterPosition * mFontBitmapSize.width;
    const auto pixelPosY = data::tilesToPixels(y) -
      (mFontBitmapSize.height - data::GameTraits::tileSize);
    mFontTexture.render(
      mpRenderer,
      {pixelPosX, pixelPosY},
      {
        {index * mFontBitmapSize.width, 0},
        mFontBitmapSize
      });
  }
}


void MenuElementRenderer::drawMessageBox(
  int x,
  int y,
  int width,
  int height
) const {
  // Top border
  drawMessageBoxRow(x, y, width, 0, 1, 2);

  // Body with left and right borders
  for (int row = 1; row < height - 1; ++row) {
    drawMessageBoxRow(x, y + row, width, 7, 8, 3);
  }

  // Bottom border
  drawMessageBoxRow(x, y + height - 1, width, 6, 5, 4);
}


void MenuElementRenderer::drawCheckBox(
  const int x,
  const int y,
  const bool isChecked
) const {
  const auto offset = isChecked ? 2 : 0;
  const auto index = 7*mSpriteSheetRenderer.tilesPerRow() + 20 + offset;

  mSpriteSheetRenderer.renderTileQuad(index, base::Vector{x - 1, y - 1});
}


void MenuElementRenderer::drawBonusScreenText(
  const int x,
  const int y,
  const std::string& text
) const {
 //        col 0, row 0: ASCII chars 48-57, 65-74
 //        col 0, row 2: ASCII chars 75-90,37,61,46,33

  for (auto i=0u; i<text.size(); ++i) {
    const auto ch = static_cast<uint8_t>(text[i]);

    int spriteSheetIndex = 0;
    if (ch >= 48 && ch <= 57) {
      spriteSheetIndex = (ch - 48) * 2;
    } else if (ch >= 65 && ch <= 74) {
      spriteSheetIndex = 20 + (ch - 65) * 2;
    } else if (ch >= 75 && ch <= 90) {
      spriteSheetIndex =
        mSpriteSheetRenderer.tilesPerRow() * 2 + (ch - 75) * 2;
    } else if (ch == 37) {
      spriteSheetIndex = 112;
    } else if (ch == 61) {
      spriteSheetIndex = 114;
    } else if (ch == 46) {
      spriteSheetIndex = 116;
    } else if (ch == 33) {
      spriteSheetIndex = 118;
    } else {
      // Skip non-renderable chars
      continue;
    }

    const auto index = static_cast<int>(i);
    mSpriteSheetRenderer.renderTileQuad(
      spriteSheetIndex, base::Vector{x + index*2, y});
  }
}


void MenuElementRenderer::drawSelectionIndicator(
  const int y,
  const int state
) const {
  const auto index = 9*mSpriteSheetRenderer.tilesPerRow() + state*2;
  mSpriteSheetRenderer.renderTileQuad(index, base::Vector{8, y - 1});
}


void MenuElementRenderer::drawTextEntryCursor(
  const int x,
  const int y,
  const int state
) const {

}


void MenuElementRenderer::drawMessageBoxRow(
  const int x,
  const int y,
  const int width,
  const int leftIndex,
  const int middleIndex,
  const int rightIndex
) const {
  const auto baseIndex = 4*40;

  mSpriteSheetRenderer.renderTile(baseIndex + leftIndex, x, y);

  const auto untilX = x + width - 1;
  for (int col = x + 1; col < untilX; ++col) {
    mSpriteSheetRenderer.renderTile(baseIndex + middleIndex, col, y);
  }

  mSpriteSheetRenderer.renderTile(baseIndex + rightIndex, x + width - 1, y);
}


void MenuElementRenderer::showMenuSelectionIndicator(int y) {
  mMenuSelectionIndicatorPosition = y;
  mPendingMenuIndicatorErase = false;
}


void MenuElementRenderer::hideMenuSelectionIndicator() {
  mPendingMenuIndicatorErase = true;
}


void MenuElementRenderer::updateAndRenderAnimatedElements(
  const engine::TimeDelta timeDelta
) {
  mElapsedTime += timeDelta;

  if (mMenuSelectionIndicatorPosition) {
    const auto yPos = *mMenuSelectionIndicatorPosition;

    if (!mPendingMenuIndicatorErase) {
      // This timing is approximate
      const auto rotations = engine::timeToFastTicks(mElapsedTime) / 15.0;
      const auto intRotations = static_cast<int>(std::round(rotations));

      drawSelectionIndicator(yPos, intRotations % NUM_MENU_INDICATOR_STATES);
    } else {
      drawSelectionIndicator(yPos, MENU_INDICATOR_STATE_FOR_CLEARING);
      mMenuSelectionIndicatorPosition = boost::none;
      mPendingMenuIndicatorErase = false;
    }
  }
}


}}
