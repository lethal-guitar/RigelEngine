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

#include "base/math_tools.hpp"

#include <dbopl.h>

#include <algorithm>
#include <array>
#include <cstdint>


namespace rigel { namespace loader {

class AdlibEmulator {
public:
  explicit AdlibEmulator(int sampleRate)
    : mEmulator(sampleRate)
  {
    // This is normally done by the game to select the right type of wave forms.
    // It's not part of the IMF files.
    mEmulator.WriteReg(1, 32);
  }

  void writeRegister(const std::uint32_t reg, const std::uint8_t value) {
    mEmulator.WriteReg(reg, value);
  }

  template<typename OutputIt>
  void render(
    std::size_t numSamples,
    OutputIt destination,
    const int volumeScale = 1
  ) {
    // DBOPL outputs 32 bit samples, but they never exceed the 16 bit range
    // (compare source code comment in MixerChannel::AddSamples() in mixer.cpp
    // in the DosBox source). Still, this means we cannot render directly into
    // the output buffer.

    while (numSamples > 0) {
      const auto samplesForIteration = std::min(mTempBuffer.size(), numSamples);

      mEmulator.GenerateBlock2(
        static_cast<DBOPL::Bitu>(samplesForIteration), mTempBuffer.data());
      destination = std::transform(
        mTempBuffer.cbegin(),
        mTempBuffer.cbegin() + samplesForIteration,
        destination,
        [volumeScale](const auto sample32Bit) {
          return static_cast<std::int16_t>(
            base::clamp(sample32Bit * volumeScale, -16384, 16384));
        });

      numSamples -= samplesForIteration;
    }
  }

private:
  DBOPL::Chip mEmulator;
  std::array<std::int32_t, 256> mTempBuffer;
};

}}
