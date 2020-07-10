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
#include "data/sound_ids.hpp"
#include "sdl_utils/ptr.hpp"

#include <array>
#include <memory>


namespace rigel::loader { class ResourceLoader; }


namespace rigel::engine {

class ImfPlayer;


class SoundSystem {
public:
  explicit SoundSystem(const loader::ResourceLoader& resources);
  ~SoundSystem();

  void playSong(data::Song&& song);
  void stopMusic() const;

  void playSound(data::SoundId id) const;
  void stopSound(data::SoundId id) const;

  void setMusicVolume(float volume);
  void setSoundVolume(float volume);

private:
  struct LoadedSound {
    LoadedSound() = default;
    explicit LoadedSound(data::AudioBuffer buffer);

    data::AudioBuffer mBuffer;
    sdl_utils::Ptr<Mix_Chunk> mpMixChunk;
  };

  LoadedSound prepareSound(const data::AudioBuffer& buffer);

  std::array<LoadedSound, data::NUM_SOUND_IDS> mSounds;
  std::unique_ptr<ImfPlayer> mpMusicPlayer;
};

}
