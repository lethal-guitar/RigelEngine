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

#include <loader/file_utils.hpp>
#include <utils/math_tools.hpp>

#include <dbopl.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <vector>


namespace rigel { namespace loader {

using namespace std;


namespace {

const auto DUKE2_IMF_RATE = 280;
const auto VOLUME_SCALE = 1;


struct ImfEntry {
  explicit ImfEntry(LeStreamReader& reader)
    : reg(reader.readU8())
    , value(reader.readU8())
    , delay(reader.readU16())
  {
  }

  const std::uint8_t reg;
  const std::uint8_t value;
  const std::uint16_t delay;
};

}

data::AudioBuffer renderImf(const ByteBuffer& imfData, const int sampleRate) {
  assert(sampleRate > 0);

  data::AudioBuffer renderedAudio{sampleRate, {}};
  // Allocate enough for 30 seconds of audio, to reduce the number of
  // reallocations during rendering
  renderedAudio.mSamples.reserve(30 * sampleRate);

  DBOPL::Chip emulator(sampleRate);
  // This is normally done by the game to select the right type of wave forms.
  // It's not part of the IMF files.
  emulator.WriteReg(1, 32);

  const auto outputSamplesPerImfTick =
    static_cast<double>(sampleRate) / DUKE2_IMF_RATE;

  // DBOPL outputs 32 bit samples, but they never exceed the 16 bit range
  // (compare source code comment in MixerChannel::AddSamples() in mixer.cpp
  // in the DosBox source). Still, this means we cannot render directly into
  // the output buffer.
  vector<std::int32_t> tempBuffer;

  LeStreamReader reader(imfData);
  while (reader.hasData()) {
    ImfEntry entry(reader);
    emulator.WriteReg(entry.reg, entry.value);

    if (entry.delay > 0) {
      const auto numSamplesToCompute = static_cast<size_t>(std::round(
        entry.delay * outputSamplesPerImfTick));
      if (tempBuffer.size() < numSamplesToCompute) {
        tempBuffer.resize(numSamplesToCompute * 2);
      }

      emulator.GenerateBlock2(
        static_cast<DBOPL::Bitu>(numSamplesToCompute), tempBuffer.data());
      std::transform(
        tempBuffer.cbegin(),
        tempBuffer.cbegin() + numSamplesToCompute,
        std::back_inserter(renderedAudio.mSamples),
        [](const auto sample32Bit) {
          return static_cast<std::int16_t>(
            utils::clamp(sample32Bit * VOLUME_SCALE, -16384, 16384));
        });
    }
  }

  return renderedAudio;
}


}}
