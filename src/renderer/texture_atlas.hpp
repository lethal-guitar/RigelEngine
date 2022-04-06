/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "data/image.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"


namespace rigel::renderer
{

/** Combines multiple images into a single texture
 *
 * For more efficient rendering, we want to minimize the number of
 * textures used each frame, as switching textures is expensive.
 * This class helps with that by combining multiple images into a single
 * large texture. We can then draw individual images by using the
 * corresponding part of the large texture.
 */
class TextureAtlas
{
public:
  /** Build a texture atlas
   *
   * Create an atlas using the provided list of images. Might use more than
   * one texture internally if not all images fit into a single texture.
   *
   * Note that the order of images in the given list determines how
   * to address these images when drawing: The first image in the list is
   * referenced by index 0, the 2nd one by index 1, etc.
   */
  TextureAtlas(Renderer* pRenderer, const std::vector<data::Image>& images);

  /** Draw image from atlas at given location
   *
   * The index parameter corresponds to the index in the list given on
   * construction.
   */
  void draw(int index, const base::Rect<int>& destRect) const;

  /** Draw (part of) image from atlas at given location
   *
   * Like the other overload of draw(), but allows specifying a source
   * rectangle to draw just a part of the specified image.
   */
  void draw(
    int index,
    const base::Rect<int>& srcRect,
    const base::Rect<int>& destRect) const;

private:
  struct TextureInfo
  {
    base::Rect<int> mRect;
    int mTextureIndex;
  };

  std::vector<TextureInfo> mAtlasMap;
  std::vector<Texture> mAtlasTextures;
  Renderer* mpRenderer;
};

} // namespace rigel::renderer
