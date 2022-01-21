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

#include "music_loader.hpp"

#include "assets/file_utils.hpp"


namespace rigel::assets
{

namespace
{

data::ImfCommand readCommand(LeStreamReader& reader)
{
  return {reader.readU8(), reader.readU8(), reader.readU16()};
}

} // namespace

data::Song loadSong(const ByteBuffer& imfData)
{
  data::Song song;

  LeStreamReader reader(imfData);
  while (reader.hasData())
  {
    song.push_back(readCommand(reader));
  }

  return song;
}

} // namespace rigel::assets
