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

#include "audio_package.hpp"

#include "assets/file_utils.hpp"


namespace rigel::assets
{

namespace
{

struct AudioDictEntry
{
  AudioDictEntry(const std::size_t offset, const std::size_t size)
    : mOffset(offset)
    , mSize(size)
  {
  }

  std::size_t mOffset;
  std::size_t mSize;
};


std::vector<AudioDictEntry> readAudioDict(const ByteBuffer& data)
{
  const auto numOffsets = data.size() / sizeof(uint32_t);

  std::vector<AudioDictEntry> dict;
  dict.reserve(numOffsets);

  LeStreamReader reader(data);
  auto previousOffset = reader.readU32();
  for (auto i = 1u; i < numOffsets; ++i)
  {
    const auto nextOffset = reader.readU32();

    const auto chunkSize = static_cast<int64_t>(nextOffset) - previousOffset;

    if (chunkSize > 0)
    {
      dict.emplace_back(previousOffset, static_cast<size_t>(chunkSize));
    }
    else if (chunkSize < 0 && i == numOffsets - 1)
    {
      dict.back().mSize = nextOffset - dict.back().mOffset;
    }

    previousOffset = nextOffset;
  }

  return dict;
}


AdlibSound parseAdlibSound(LeStreamReader& reader)
{
  AdlibSound sound;

  const auto length = reader.readU32();
  sound.mSoundData.reserve(length);
  reader.skipBytes(sizeof(uint16_t)); // priority - not interesting for us
  for (auto& setting : sound.mInstrumentSettings)
  {
    setting = reader.readU8();
  }
  sound.mOctave = reader.readU8();

  for (auto i = 0u; i < length; ++i)
  {
    sound.mSoundData.push_back(reader.readU8());
  }

  return sound;
}

} // namespace


AudioPackage loadAdlibSoundData(
  const ByteBuffer& audioDictData,
  const ByteBuffer& bundledAudioData)
{
  AudioPackage sounds;

  const auto audioDict = readAudioDict(audioDictData);
  if (audioDict.size() < 68u)
  {
    throw std::invalid_argument("Corrupt Duke Nukem II AUDIOT/AUDIOHED");
  }

  for (auto i = 34u; i < 68u; ++i)
  {
    const auto& dictEntry = audioDict[i];

    const auto soundStartIter = bundledAudioData.begin() + dictEntry.mOffset;
    LeStreamReader reader(soundStartIter, soundStartIter + dictEntry.mSize);
    sounds.emplace_back(parseAdlibSound(reader));
  }

  return sounds;
}

} // namespace rigel::assets
