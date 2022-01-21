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

#include "assets/byte_buffer.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>


namespace rigel::assets
{


class CMPFilePackage
{
public:
  explicit CMPFilePackage(const std::string& filePath);

  ByteBuffer file(std::string_view name) const;

  bool hasFile(std::string_view name) const;

private:
  struct DictEntry
  {
    DictEntry(std::uint32_t fileOffset, std::uint32_t fileSize);
    DictEntry(const DictEntry&) = default;
    DictEntry(DictEntry&&) = default;

    const std::uint32_t fileOffset = 0;
    const std::uint32_t fileSize = 0;
  };

  using FileDict = std::unordered_map<std::string, DictEntry>;

  FileDict::const_iterator findFileEntry(std::string_view name) const;

private:
  std::vector<std::uint8_t> mFileData;
  FileDict mFileDict;
};


} // namespace rigel::assets
