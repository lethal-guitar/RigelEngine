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

#include "base/defer.hpp"
#include "data/audio_buffer.hpp"
#include "data/game_options.hpp"
#include "data/song.hpp"
#include "data/sound_ids.hpp"
#include "sdl_utils/ptr.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>


namespace rigel::loader
{
class ResourceLoader;
}


namespace rigel::audio
{

class ImfPlayer;


using RawBuffer = std::vector<std::uint8_t>;


/** Provides sound and music playback functionality
 *
 * This class implements sound and music playback. When constructed, it opens
 * an audio device and loads all sound effects from the game's data files. From
 * that point on, sound effects and music playback can be triggered at any time
 * using the class' interface. Sound and music volume can also be adjusted.
 */
class SoundSystem
{
public:
  explicit SoundSystem(
    const loader::ResourceLoader* pResources,
    data::SoundStyle soundStyle);
  ~SoundSystem();

  void reloadAllSounds(data::SoundStyle soundStyle);

  /** Start playing given music data
   *
   * Starts playback of the song identified by the given name, and returns
   * immediately. Music plays in parallel to any sound effects.
   */
  void playSong(const std::string& name);

  /** Stop playing current song (if playing) */
  void stopMusic() const;

  /** Start playing specified sound effect
   *
   * Starts playback of the sound effect specified by the given sound ID, and
   * returns immediately. The sound effect will play in parallel to any other
   * currently playing sound effects, unless the same sound ID is already
   * playing. In the latter case, the already playing sound effect will be cut
   * off and playback will restart from the beginning.
   */
  void playSound(data::SoundId id) const;

  /** Stop playing specified sound effect (if currently playing) */
  void stopSound(data::SoundId id) const;
  void stopAllSounds() const;

  void setMusicVolume(float volume);
  void setSoundVolume(float volume);

private:
  void loadAllSounds(
    int sampleRate,
    std::uint16_t audioFormat,
    int numChannels,
    data::SoundStyle soundStyle);
  data::AudioBuffer loadSoundForStyle(
    data::SoundId id,
    data::SoundStyle soundStyle,
    int sampleRate) const;
  void applySoundVolume(float volume);
  void hookMusic() const;
  void unhookMusic() const;
  sdl_utils::Ptr<Mix_Music> loadReplacementSong(const std::string& name);

  struct MusicConversionWrapper;

  struct LoadedSound
  {
    LoadedSound() = default;
    explicit LoadedSound(RawBuffer buffer);
    explicit LoadedSound(sdl_utils::Ptr<Mix_Chunk> pMixChunk);

    RawBuffer mData;
    sdl_utils::Ptr<Mix_Chunk> mpMixChunk;
  };

  base::ScopeGuard mCloseMixerGuard;
  std::array<LoadedSound, data::NUM_SOUND_IDS> mSounds;
  std::unique_ptr<ImfPlayer> mpMusicPlayer;
  std::unique_ptr<MusicConversionWrapper> mpMusicConversionWrapper;
  mutable sdl_utils::Ptr<Mix_Music> mpCurrentReplacementSong;
  mutable std::unordered_map<std::string, std::string>
    mReplacementSongFileCache;
  const loader::ResourceLoader* mpResources;
  float mCurrentSoundVolume;
  data::SoundStyle mCurrentSoundStyle;
};

} // namespace rigel::audio
