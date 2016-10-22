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

#include <loader/file_utils.hpp>

#include <dbopl.h>
#include <algorithm>
#include <iostream>


namespace rigel { namespace loader {

using namespace std;
using data::SoundId;

namespace {

const auto ADLIB_SOUND_RATE = 140;


struct AudioDictEntry {
  AudioDictEntry(
    const std::size_t offset,
    const std::size_t size
  )
    : mOffset(offset)
    , mSize(size)
  {
  }

  std::size_t mOffset;
  std::size_t mSize;
};


std::vector<AudioDictEntry> readAudioDict(const ByteBuffer& data) {
  const auto numOffsets = data.size() / sizeof(uint32_t);

  vector<AudioDictEntry> dict;
  dict.reserve(numOffsets);

  LeStreamReader reader(data);
  auto previousOffset = reader.readU32();
  for (auto i=1u; i<numOffsets; ++i) {
    const auto nextOffset = reader.readU32();

    const auto chunkSize = static_cast<int64_t>(nextOffset) - previousOffset;

    if (chunkSize > 0) {
      dict.emplace_back(previousOffset, chunkSize);
    } else if (chunkSize < 0 && i == numOffsets - 1) {
      dict.back().mSize = nextOffset - dict.back().mOffset;
    }

    previousOffset = nextOffset;
  }

  return dict;
}

}


AudioPackage::AdlibSound::AdlibSound(LeStreamReader& reader) {
  const auto length = reader.readU32();
  mSoundData.reserve(length);
  reader.skipBytes(sizeof(uint16_t)); // priority - not interesting for us
  for (auto& setting : mInstrumentSettings) {
    setting = reader.readU8();
  }
  mOctave = reader.readU8();

  for (auto i=0u; i<length; ++i) {
    mSoundData.push_back(reader.readU8());
  }
}


AudioPackage::AudioPackage(const CMPFilePackage& filePackage) {
  const auto audioDict = readAudioDict(filePackage.file("AUDIOHED.MNI"));
  if (audioDict.size() < 68u) {
    throw std::invalid_argument("Corrupt Duke Nukem II AUDIOT/AUDIOHED");
  }

  const auto bundledAudioData = filePackage.file("AUDIOT.MNI");
  for (auto i=34u; i<68u; ++i) {
    const auto& dictEntry = audioDict[i];

    const auto soundStartIter = bundledAudioData.cbegin() + dictEntry.mOffset;
    LeStreamReader reader(soundStartIter, soundStartIter + dictEntry.mSize);
    mSounds.emplace_back(reader);
  }
}


data::AudioBuffer AudioPackage::loadAdlibSound(SoundId id) const {
  const auto idAsIndex = static_cast<int>(id);
  if (idAsIndex < 0 || idAsIndex >= 34) {
    throw std::invalid_argument("Invalid sound ID");
  }

  const auto& sound = mSounds[idAsIndex];
  return renderAdlibSound(sound);
}


data::AudioBuffer AudioPackage::renderAdlibSound(
  const AdlibSound& sound
) const {
  const auto sampleRate = 44100;
  const auto samplesPerTick = sampleRate / ADLIB_SOUND_RATE;

  DBOPL::Chip emulator(sampleRate);
  emulator.WriteReg(0x01, 0x20);

  emulator.WriteReg(0x20, sound.mInstrumentSettings[0]);
  emulator.WriteReg(0x40, sound.mInstrumentSettings[2]);
  emulator.WriteReg(0x60, sound.mInstrumentSettings[4]);
  emulator.WriteReg(0x80, sound.mInstrumentSettings[6]);
  emulator.WriteReg(0xE0, sound.mInstrumentSettings[8]);

  emulator.WriteReg(0x23, sound.mInstrumentSettings[1]);
  emulator.WriteReg(0x43, sound.mInstrumentSettings[3]);
  emulator.WriteReg(0x63, sound.mInstrumentSettings[5]);
  emulator.WriteReg(0x83, sound.mInstrumentSettings[7]);
  emulator.WriteReg(0xE3, sound.mInstrumentSettings[9]);

  emulator.WriteReg(0xC0, 0);
  emulator.WriteReg(0xB0, 0);

  const auto octaveBits = static_cast<uint8_t>((sound.mOctave & 7) << 2);

  vector<data::Sample> renderedSamples;
  renderedSamples.reserve(sound.mSoundData.size() * samplesPerTick);
  vector<int32_t> tempBuffer(samplesPerTick);

  for (const auto byte : sound.mSoundData) {
    if (byte == 0) {
      emulator.WriteReg(0xB0, 0);
    } else {
      emulator.WriteReg(0xA0, byte);
      emulator.WriteReg(0xB0, 0x20 | octaveBits);
    }

    emulator.GenerateBlock2(samplesPerTick, tempBuffer.data());
    std::transform(
      tempBuffer.cbegin(),
      tempBuffer.cend(),
      back_inserter(renderedSamples),
      [](const auto sample) {
        return static_cast<data::Sample>(sample * 2);
      });
  }

  return {sampleRate, renderedSamples};
}

}}
