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

#include <base/image.hpp>
#include <base/spatial_types.hpp>
#include <vector>


namespace rigel::data
{

struct MovieFrame
{
  MovieFrame(Image&& replacementImage, const int startRow)
    : mReplacementImage(std::move(replacementImage))
    , mStartRow(startRow)
  {
  }

  Image mReplacementImage;
  int mStartRow;
};


struct Movie
{
  Image mBaseImage;
  std::vector<MovieFrame> mFrames;
};

} // namespace rigel::data
