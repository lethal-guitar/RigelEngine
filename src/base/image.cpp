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

#include "image.hpp"

#include <stdexcept>


namespace rigel::data
{

using namespace std;


Image::Image(
  const PixelBuffer& pixels,
  const std::size_t width,
  const std::size_t height)
  : mPixels(pixels)
  , mWidth(width)
  , mHeight(height)
{
}


Image::Image(
  PixelBuffer&& pixels,
  const std::size_t width,
  const std::size_t height)
  : mPixels(std::move(pixels))
  , mWidth(width)
  , mHeight(height)
{
}


Image::Image(const std::size_t width, const std::size_t height)
  : Image(vector<Pixel>(width * height, Pixel{}), width, height)
{
}


Image Image::flipped() const
{
  PixelBuffer flippedPixelData;
  flippedPixelData.resize(width() * height());

  for (std::size_t y = 0; y < height(); ++y)
  {
    const auto sourceRow = height() - (y + 1);
    const auto yOffsetSource = sourceRow * width();
    const auto yOffset = y * width();
    for (std::size_t x = 0; x < width(); ++x)
    {
      flippedPixelData[x + yOffset] = pixelData()[x + yOffsetSource];
    }
  }

  return Image{std::move(flippedPixelData), width(), height()};
}


void Image::insertImage(const size_t x, const size_t y, const Image& image)
{
  insertImage(x, y, image.pixelData(), image.width());
}


void Image::insertImage(
  const size_t x,
  const size_t y,
  const PixelBuffer& pixels,
  const size_t sourceWidth)
{
  const auto inferredHeight = pixels.size() / sourceWidth;
  if (x + sourceWidth > mWidth || y + inferredHeight > mHeight)
  {
    throw invalid_argument("Source image doesn't fit");
  }

  auto sourceIter = pixels.begin();
  for (size_t row = 0; row < inferredHeight; ++row)
  {
    for (size_t col = 0; col < sourceWidth; ++col)
    {
      const auto targetOffset = (x + col) + (y + row) * mWidth;
      mPixels[targetOffset] = *sourceIter++;
    }
  }
}


} // namespace rigel::data
