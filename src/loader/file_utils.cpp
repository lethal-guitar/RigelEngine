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

#include "file_utils.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>


namespace rigel { namespace loader {

using namespace std;


namespace {

const char* OUT_OF_DATA_ERROR_MSG = "No more data in stream";

}


ByteBuffer loadFile(const string& fileName) {
  ifstream file(fileName, ios::binary | ios::ate);
  if (!file.is_open()) {
    throw runtime_error(string("File can't be opened: ") + fileName);
  }

  const auto fileSize = file.tellg();
  file.seekg(0);
  ByteBuffer data(fileSize);
  static_assert(sizeof(char) == sizeof(uint8_t), "");
  file.read(reinterpret_cast<char*>(data.data()), fileSize);
  return data;
}


LeStreamReader::LeStreamReader(const ByteBuffer& data)
  : LeStreamReader(data.cbegin(), data.cend())
{
}


LeStreamReader::LeStreamReader(ByteBufferCIter begin, ByteBufferCIter end)
  : mCurrentByteIter(begin)
  , mDataEnd(end)
{
}


uint8_t LeStreamReader::readU8() {
  if (mCurrentByteIter == mDataEnd) {
    throw runtime_error(OUT_OF_DATA_ERROR_MSG);
  }
  return *mCurrentByteIter++;
}


uint16_t LeStreamReader::readU16() {
  const auto lsb = readU8();
  const auto msb = readU8();
  return lsb | uint16_t(msb << 8);
}


uint32_t LeStreamReader::readU24() {
  const auto lsb = readU8();
  const auto middle = readU8();
  const auto msb = readU8();

  return lsb | (middle << 8) | (msb << 16);
}


uint32_t LeStreamReader::readU32() {
  const auto lowWord = readU16();
  const auto highWord = readU16();
  return lowWord | (highWord << 16);
}


int8_t LeStreamReader::readS8() {
  return static_cast<int8_t>(readU8());
}


int16_t LeStreamReader::readS16() {
  return static_cast<int16_t>(readU16());
}


int32_t LeStreamReader::readS24() {
  static_assert(static_cast<int8_t>(0xFFU) == -1, "Need two's complement");
  const auto rawValue = readU24();
  const auto extension = (rawValue & 0x800000) ? 0xFF : 0x00;
  return static_cast<int32_t>((extension << 24) | (rawValue & 0xFFFFFF));
}


int32_t LeStreamReader::readS32() {
  return static_cast<int32_t>(readU32());
}


void LeStreamReader::skipBytes(const size_t count) {
  assert(distance(mCurrentByteIter, mDataEnd) >= 0);
  const auto availableBytes = static_cast<size_t>(
    distance(mCurrentByteIter, mDataEnd));
  if (availableBytes < count) {
    throw runtime_error(OUT_OF_DATA_ERROR_MSG);
  }

  advance(mCurrentByteIter, count);
}


bool LeStreamReader::hasData() const {
  return mCurrentByteIter != mDataEnd;
}


ByteBufferCIter LeStreamReader::currentIter() const {
  return mCurrentByteIter;
}


string readFixedSizeString(LeStreamReader& reader, const size_t len) {
  vector<char> characters;
  for (size_t i=0; i<len; ++i) {
    characters.push_back(static_cast<char>(reader.readU8()));
  }

  // Ensure string is zero-terminated
  characters.push_back(0);

  // Let std::string's ctor handle the zero-terminator detection
  return string(characters.data());
}

}}
