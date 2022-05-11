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

#include "cmp_file_package.hpp"

#include "assets/file_utils.hpp"
#include "base/string_utils.hpp"

#include <cassert>
#include <stdexcept>


namespace rigel::assets
{

using namespace std;


namespace
{

std::string normalizedFileName(std::string_view fileName)
{
  return strings::toUppercase(fileName);
}

} // namespace


CMPFilePackage::CMPFilePackage(const std::filesystem::path& filePath)
  : mFileData(loadFile(filePath))
{
  LeStreamReader dictReader(mFileData.begin(), mFileData.end());

  while (dictReader.hasData())
  {
    const auto fileName = readFixedSizeString(dictReader, 12);
    const auto fileOffset = dictReader.readU32();
    const auto fileSize = dictReader.readU32();

    if (fileOffset == 0 && fileSize == 0)
    {
      break;
    }
    if (fileOffset + fileSize > mFileData.size())
    {
      throw invalid_argument("Malformed dictionary in CMP file");
    }

    mFileDict.emplace(
      normalizedFileName(fileName), DictEntry(fileOffset, fileSize));
  }
}


ByteBuffer CMPFilePackage::file(std::string_view name) const
{
  const auto it = findFileEntry(name);
  if (it == mFileDict.end())
  {
    throw invalid_argument(
      string("No such file in CMP: ") + normalizedFileName(name));
  }

  const auto& fileHeader = it->second;
  const auto fileStart = mFileData.begin() + fileHeader.fileOffset;
  return ByteBuffer(fileStart, fileStart + fileHeader.fileSize);
}


bool CMPFilePackage::hasFile(std::string_view name) const
{
  return findFileEntry(name) != mFileDict.end();
}


CMPFilePackage::FileDict::const_iterator
  CMPFilePackage::findFileEntry(std::string_view name) const
{
  return mFileDict.find(normalizedFileName(name));
}


CMPFilePackage::DictEntry::DictEntry(
  const uint32_t fileOffset_,
  const uint32_t fileSize_)
  : fileOffset(fileOffset_)
  , fileSize(fileSize_)
{
}


} // namespace rigel::assets
