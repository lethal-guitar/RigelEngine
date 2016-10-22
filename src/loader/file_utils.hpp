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

#include <loader/byte_buffer.hpp>

#include <cstdint>
#include <string>


namespace rigel { namespace loader {

/** Load entire contents of file with given name into ByteBuffer
 *
 * Throws an exception if the file can't be opened.
 */
ByteBuffer loadFile(const std::string& fileName);


/** Offers checked reading of little-endian data from a byte buffer
 *
 * All readX() methods will throw if there is not enough data left.
 */
class LeStreamReader {
public:
  explicit LeStreamReader(const ByteBuffer& data);
  LeStreamReader(ByteBufferCIter begin, ByteBufferCIter end);

  std::uint8_t readU8();
  std::uint16_t readU16();
  /** Read 32bit little-endian word encoded as 3 bytes */
  std::uint32_t readU24();
  std::uint32_t readU32();

  std::int8_t readS8();
  std::int16_t readS16();
  /** Read 32bit little-endian word encoded as 3 bytes */
  std::int32_t readS24();
  std::int32_t readS32();

  void skipBytes(std::size_t count);
  bool hasData() const;
  ByteBufferCIter currentIter() const;

private:
  ByteBufferCIter mCurrentByteIter;
  const ByteBufferCIter mDataEnd;
};


std::string readFixedSizeString(LeStreamReader& reader, std::size_t len);


}}
