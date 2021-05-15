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

#include "data/audio_buffer.hpp"
#include "data/sound_ids.hpp"
#include "loader/byte_buffer.hpp"

#include <array>
#include <cstdint>


namespace rigel::loader
{

class LeStreamReader;

class AudioPackage
{
public:
  static constexpr auto AUDIO_DICT_FILE = "AUDIOHED.MNI";
  static constexpr auto AUDIO_DATA_FILE = "AUDIOT.MNI";

  AudioPackage(
    const ByteBuffer& audioDictData,
    const ByteBuffer& bundledAudioData);

  data::AudioBuffer loadAdlibSound(data::SoundId id) const;

private:
  struct AdlibSound
  {
    explicit AdlibSound(LeStreamReader& reader);

    std::uint8_t mOctave = 0;
    std::array<std::uint8_t, 16> mInstrumentSettings;
    std::vector<std::uint8_t> mSoundData;
  };

  data::AudioBuffer renderAdlibSound(const AdlibSound& sound) const;

private:
  std::vector<AdlibSound> mSounds;
};


} // namespace rigel::loader
