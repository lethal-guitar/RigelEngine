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
#include "sdl_utils/ptr.hpp"

#include <unordered_map>
#include <vector>


namespace rigel { namespace engine {

class SoundSystem {
public:
  using SoundHandle = int;

  SoundSystem();
  ~SoundSystem();

  SoundHandle addSong(data::AudioBuffer buffer);
  SoundHandle addSound(const data::AudioBuffer& buffer);

  void reportMemoryUsage() const;

  void playSong(SoundHandle handle) const;
  void stopMusic() const;

  /** Play sound with specified volume and panning
   *
   * volume range is 0.0 (silent) to 1.0 (max volume)
   * pan range is -1.0 (left) to (1.0) right, 0.0 is center
   */
  void playSound(
    SoundHandle handle,
    float volume = 1.0f,
    float pan = 0.0f) const;

  void clearAll();

private:
  std::vector<data::AudioBuffer> mAudioBuffers;
  std::unordered_map<SoundHandle, sdl_utils::Ptr<Mix_Chunk>> mLoadedChunks;
  SoundHandle mNextHandle = 0;
  mutable int mCurrentSongChannel = -1;
  mutable int mCurrentSoundChannel = -1;
};

}}
