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
#include "data/song.hpp"
#include "sdl_utils/ptr.hpp"

#include <memory>
#include <unordered_map>
#include <vector>


namespace rigel { namespace engine {

class ImfPlayer;


class SoundSystem {
public:
  using SoundHandle = int;

  SoundSystem();
  ~SoundSystem();

  SoundHandle addSound(const data::AudioBuffer& buffer);

  void playSong(data::Song&& song);
  void stopMusic() const;

  void playSound(SoundHandle handle) const;

  void clearAll();

private:
  SoundHandle addConvertedSound(data::AudioBuffer buffer);

  std::unique_ptr<ImfPlayer> mpMusicPlayer;
  std::vector<data::AudioBuffer> mAudioBuffers;
  std::unordered_map<SoundHandle, sdl_utils::Ptr<Mix_Chunk>> mLoadedChunks;
  SoundHandle mNextHandle = 0;
  mutable int mCurrentSoundChannel = -1;
};

}}
