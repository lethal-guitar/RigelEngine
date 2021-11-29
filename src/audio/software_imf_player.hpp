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

#include "data/song.hpp"
#include "loader/adlib_emulator.hpp"

#include <atomic>
#include <mutex>


namespace rigel::audio
{

class SoftwareImfPlayer
{
public:
  explicit SoftwareImfPlayer(int sampleRate);
  SoftwareImfPlayer(const SoftwareImfPlayer&) = delete;
  SoftwareImfPlayer& operator=(const SoftwareImfPlayer&) = delete;

  void playSong(data::Song&& song);
  void setVolume(const float volume);

  void render(std::int16_t* pBuffer, std::size_t samplesRequired);

private:
  loader::AdlibEmulator mEmulator;
  std::mutex mAudioLock;
  data::Song mNextSongData;

  data::Song mSongData;
  data::Song::const_iterator miNextCommand;
  std::size_t mSamplesAvailable = 0;
  int mSampleRate;

  std::atomic<float> mVolume;
  std::atomic<bool> mSongSwitchPending;
};

} // namespace rigel::audio
