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

#include "imf_player.hpp"

#include "base/math_tools.hpp"
#include "data/game_traits.hpp"


namespace rigel::engine {

namespace {

int imfDelayToSamples(const int delay, const int sampleRate) {
  const auto samplesPerImfTick =
    static_cast<double>(sampleRate) / data::GameTraits::musicPlaybackRate;
  return base::round(delay * samplesPerImfTick);
}

}


ImfPlayer::ImfPlayer(const int sampleRate)
  : mEmulator(sampleRate)
  , mSampleRate(sampleRate)
  , mSongSwitchPending(false)
{
}


void ImfPlayer::playSong(data::Song&& song) {
  {
    std::lock_guard<std::mutex> takeLock{mAudioLock};
    mNextSongData = std::move(song);
  }
  mSongSwitchPending = true;
}


void ImfPlayer::render(std::int16_t* pBuffer, std::size_t samplesRequired) {
  if (mSongSwitchPending && mAudioLock.try_lock()) {
    mSongData = std::move(mNextSongData);
    mSongSwitchPending = false;
    mAudioLock.unlock();

    miNextCommand = mSongData.cbegin();
    mSamplesAvailable = 0;
  }

  if (mSongData.empty()) {
    std::fill(pBuffer, pBuffer + samplesRequired, int16_t{0});
    return;
  }

  while (samplesRequired > mSamplesAvailable) {
    mEmulator.render(mSamplesAvailable, pBuffer);
    pBuffer += mSamplesAvailable;
    samplesRequired -= mSamplesAvailable;

    auto commandDelay = 0;
    do {
      const auto& command = *miNextCommand;
      commandDelay = command.delay;
      mEmulator.writeRegister(command.reg, command.value);
      ++miNextCommand;
      if (miNextCommand == mSongData.cend()) {
        miNextCommand = mSongData.cbegin();
      }
    } while (commandDelay == 0);

    mSamplesAvailable = imfDelayToSamples(commandDelay, mSampleRate);
  }

  mEmulator.render(samplesRequired, pBuffer);
  mSamplesAvailable -= samplesRequired;
}


}
