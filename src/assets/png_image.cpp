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

#include "png_image.hpp"

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <stb_image.h>
#include <stb_image_write.h>
RIGEL_RESTORE_WARNINGS

#include <memory>

namespace rigel::assets
{

std::optional<data::Image> loadPng(const std::string& path)
{
  int width = 0;
  int height = 0;
  const auto imageDeleter = [](unsigned char* p) {
    stbi_image_free(p);
  };
  std::unique_ptr<unsigned char, decltype(imageDeleter)> pImageData{
    stbi_load(path.c_str(), &width, &height, nullptr, 4), imageDeleter};

  if (pImageData)
  {
    const auto pPixels = reinterpret_cast<const data::Pixel*>(pImageData.get());
    const auto numPixels = static_cast<size_t>(width * height);
    data::PixelBuffer buffer{pPixels, pPixels + numPixels};
    return data::Image{
      std::move(buffer),
      static_cast<size_t>(width),
      static_cast<size_t>(height)};
  }

  return {};
}

bool savePng(const std::string& path, const data::Image& image)
{
  const auto width = static_cast<int>(image.width());
  const auto height = static_cast<int>(image.height());
  const auto result = stbi_write_png(
    path.c_str(), width, height, 4, image.pixelData().data(), width * 4);
  return result != 0;
}

} // namespace rigel::assets
