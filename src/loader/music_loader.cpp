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

#include "data/song.hpp"
#include "loader/adlib_emulator.hpp"
#include "loader/file_utils.hpp"

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


data::ImfCommand readCommand(LeStreamReader& reader) {
  return {
    reader.readU8(),
    reader.readU8(),
    reader.readU16()};
}

}


data::AudioBuffer renderImf(const ByteBuffer& imfData, const int sampleRate) {
  assert(sampleRate > 0);

  data::AudioBuffer renderedAudio{sampleRate, {}};
  // Allocate enough for 30 seconds of audio, to reduce the number of
  // reallocations during rendering
  renderedAudio.mSamples.reserve(30 * sampleRate);

  AdlibEmulator emulator{sampleRate};

  const auto outputSamplesPerImfTick =
    static_cast<double>(sampleRate) / DUKE2_IMF_RATE;

  LeStreamReader reader(imfData);
  while (reader.hasData()) {
    const auto entry = readCommand(reader);
    emulator.writeRegister(entry.reg, entry.value);

    if (entry.delay > 0) {
      const auto numSamplesToCompute = static_cast<size_t>(std::round(
        entry.delay * outputSamplesPerImfTick));
      emulator.render(
        numSamplesToCompute, std::back_inserter(renderedAudio.mSamples));
    }
  }

  return renderedAudio;
}


}}
