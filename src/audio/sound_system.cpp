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

#include "sound_system.hpp"

#include "assets/audio_package.hpp"
#include "assets/resource_loader.hpp"
#include "audio/adlib_emulator.hpp"
#include "audio/software_imf_player.hpp"
#include "base/math_utils.hpp"
#include "base/string_utils.hpp"
#include "sdl_utils/error.hpp"

#include <speex/speex_resampler.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <utility>


namespace rigel::sdl_mixer
{

namespace
{

void check(int result)
{
  using namespace std::string_literals;

  if (result != 0)
  {
    throw std::runtime_error{"SDL_mixer error: "s + Mix_GetError()};
  }
}

} // namespace

} // namespace rigel::sdl_mixer


namespace rigel::audio
{

namespace
{

const auto ADLIB_SOUND_RATE = 140;

const auto COMBINED_SOUNDS_ADLIB_PERCENTAGE = 0.30f;
const auto DESIRED_SAMPLE_RATE = 44100;
const auto BUFFER_SIZE = 2048;

data::AudioBuffer
  resampleAudio(const data::AudioBuffer& buffer, const int newSampleRate)
{
  using ResamplerPtr =
    std::unique_ptr<SpeexResamplerState, decltype(&speex_resampler_destroy)>;
  ResamplerPtr pResampler(
    speex_resampler_init(1, buffer.mSampleRate, newSampleRate, 5, nullptr),
    &speex_resampler_destroy);
  speex_resampler_skip_zeros(pResampler.get());

  auto inputLength = static_cast<spx_uint32_t>(buffer.mSamples.size());
  auto outputLength = static_cast<spx_uint32_t>(
    base::integerDivCeil<spx_uint32_t>(inputLength, buffer.mSampleRate) *
    newSampleRate);

  std::vector<data::Sample> resampled(outputLength);
  speex_resampler_process_int(
    pResampler.get(),
    0,
    buffer.mSamples.data(),
    &inputLength,
    resampled.data(),
    &outputLength);
  resampled.resize(outputLength);
  return {newSampleRate, resampled};
}


void appendRampToZero(data::AudioBuffer& buffer, const int sampleRate)
{
  // Roughly 10 ms of linear ramp
  const auto rampLength = (sampleRate / 100);

  buffer.mSamples.reserve(buffer.mSamples.size() + rampLength - 1);
  const auto lastSample = buffer.mSamples.back();

  for (int i = 1; i < rampLength; ++i)
  {
    const auto interpolation = i / static_cast<double>(rampLength);
    const auto rampedValue = lastSample * (1.0 - interpolation);
    buffer.mSamples.push_back(base::roundTo<data::Sample>(rampedValue));
  }
}


// Prepares the given audio buffer to be loaded into a Mix_Chunk. This includes
// resampling to the given sample rate and making sure the buffer ends in a
// zero value to avoid clicks/pops.
data::AudioBuffer
  prepareBuffer(const data::AudioBuffer& original, const int sampleRate)
{
  auto buffer = resampleAudio(original, sampleRate);
  if (buffer.mSamples.back() != 0)
  {
    // Prevent clicks/pops with samples that don't return to 0 at the end
    // by adding a small linear ramp leading back to zero.
    appendRampToZero(buffer, sampleRate);
  }

  return buffer;
}


// Converts the given audio buffer into the given audio format and returns it
// as a raw buffer
RawBuffer convertBuffer(
  const data::AudioBuffer& buffer,
  const std::uint16_t audioFormat,
  const int numChannels)
{
  SDL_AudioCVT conversionSpecs;
  SDL_BuildAudioCVT(
    &conversionSpecs,
    AUDIO_S16LSB,
    1,
    buffer.mSampleRate,
    audioFormat,
    std::uint8_t(numChannels),
    buffer.mSampleRate);

  const auto sizeInBytes = buffer.mSamples.size() * sizeof(std::int16_t);
  auto converted = RawBuffer(sizeInBytes * conversionSpecs.len_mult);
  std::memcpy(converted.data(), buffer.mSamples.data(), sizeInBytes);

  conversionSpecs.len = static_cast<int>(sizeInBytes);
  conversionSpecs.buf = converted.data();
  SDL_ConvertAudio(&conversionSpecs);

  converted.resize(conversionSpecs.len_cvt);
  converted.shrink_to_fit();

  return converted;
}


void overlaySound(
  data::AudioBuffer& base,
  const data::AudioBuffer& overlay,
  const float overlayVolume)
{
  assert(base.mSampleRate == overlay.mSampleRate);

  if (base.mSamples.size() < overlay.mSamples.size())
  {
    base.mSamples.resize(overlay.mSamples.size());
  }

  const auto overlayVolumeSdl =
    base::round(std::clamp(overlayVolume, 0.0f, 1.0f) * SDL_MIX_MAXVOLUME);

  SDL_MixAudioFormat(
    reinterpret_cast<std::uint8_t*>(base.mSamples.data()),
    reinterpret_cast<const std::uint8_t*>(overlay.mSamples.data()),
    AUDIO_S16LSB,
    static_cast<std::uint32_t>(overlay.mSamples.size() * sizeof(data::Sample)),
    overlayVolumeSdl);
}


auto idToIndex(const data::SoundId id)
{
  return static_cast<int>(id);
}

data::AudioBuffer renderAdlibSound(
  const assets::AdlibSound& sound,
  const AdlibEmulator::Type emulatorType)
{
  AdlibEmulator emulator{OPL2_SAMPLE_RATE, emulatorType};

  emulator.writeRegister(0x20, sound.mInstrumentSettings[0]);
  emulator.writeRegister(0x40, sound.mInstrumentSettings[2]);
  emulator.writeRegister(0x60, sound.mInstrumentSettings[4]);
  emulator.writeRegister(0x80, sound.mInstrumentSettings[6]);
  emulator.writeRegister(0xE0, sound.mInstrumentSettings[8]);

  emulator.writeRegister(0x23, sound.mInstrumentSettings[1]);
  emulator.writeRegister(0x43, sound.mInstrumentSettings[3]);
  emulator.writeRegister(0x63, sound.mInstrumentSettings[5]);
  emulator.writeRegister(0x83, sound.mInstrumentSettings[7]);
  emulator.writeRegister(0xE3, sound.mInstrumentSettings[9]);

  emulator.writeRegister(0xC0, 0);
  emulator.writeRegister(0xB0, 0);

  const auto octaveBits = static_cast<uint8_t>((sound.mOctave & 7) << 2);

  const auto samplesPerTick = OPL2_SAMPLE_RATE / ADLIB_SOUND_RATE;
  std::vector<data::Sample> renderedSamples;
  renderedSamples.reserve(sound.mSoundData.size() * samplesPerTick);

  for (const auto byte : sound.mSoundData)
  {
    if (byte == 0)
    {
      emulator.writeRegister(0xB0, 0);
    }
    else
    {
      emulator.writeRegister(0xA0, byte);
      emulator.writeRegister(0xB0, 0x20 | octaveBits);
    }

    emulator.render(samplesPerTick, back_inserter(renderedSamples), 2);
  }

  return {OPL2_SAMPLE_RATE, renderedSamples};
}


data::AudioBuffer loadSoundForStyle(
  const data::SoundId id,
  const data::SoundStyle soundStyle,
  const int sampleRate,
  const assets::ResourceLoader& resources,
  const assets::AudioPackage& soundPackage,
  const AdlibEmulator::Type emulatorType)
{
  auto loadAdlibSound = [&](const data::SoundId soundId) {
    const auto idAsIndex = static_cast<int>(soundId);
    if (idAsIndex < 0 || idAsIndex >= 34)
    {
      throw std::invalid_argument("Invalid sound ID");
    }

    return renderAdlibSound(soundPackage[idAsIndex], emulatorType);
  };

  auto loadPreferredSound = [&](const data::SoundId soundId) {
    const auto buffer = resources.loadSoundBlasterSound(soundId);
    if (buffer.mSamples.empty())
    {
      return loadAdlibSound(soundId);
    }

    return buffer;
  };


  if (data::isIntroSound(id))
  {
    // The intro sounds don't have AdLib versions, so always load
    // the 'preferred' version (SoundBlaster) regardless of chosen
    // sound style.
    return prepareBuffer(loadPreferredSound(id), sampleRate);
  }

  switch (soundStyle)
  {
    case data::SoundStyle::AdLib:
      return prepareBuffer(loadAdlibSound(id), sampleRate);

    case data::SoundStyle::Combined:
      {
        auto buffer = prepareBuffer(loadPreferredSound(id), sampleRate);
        if (resources.hasSoundBlasterSound(id))
        {
          overlaySound(
            buffer,
            prepareBuffer(loadAdlibSound(id), sampleRate),
            COMBINED_SOUNDS_ADLIB_PERCENTAGE);
        }

        return buffer;
      }

    default:
      return prepareBuffer(loadPreferredSound(id), sampleRate);
  }
}


AdlibEmulator::Type toEmulationType(const data::AdlibPlaybackType type)
{
  return type == data::AdlibPlaybackType::DBOPL
    ? AdlibEmulator::Type::DBOPL
    : AdlibEmulator::Type::NukedOpl3;
}

} // namespace


struct SoundSystem::ImfPlayerWrapper
{
  ImfPlayerWrapper(
    const std::uint16_t audioFormat,
    const int sampleRate,
    const int numChannels)
    : mPlayer(sampleRate)
    , mBytesPerSample((SDL_AUDIO_BITSIZE(audioFormat) / 8) * numChannels)
  {
    SDL_BuildAudioCVT(
      &mConversionSpecs,
      AUDIO_S16LSB,
      1,
      sampleRate,
      audioFormat,
      std::uint8_t(numChannels),
      sampleRate);

    const auto bufferSize =
      BUFFER_SIZE * sizeof(std::int16_t) * mConversionSpecs.len_mult;
    mpBuffer = std::unique_ptr<std::uint8_t[]>{new std::uint8_t[bufferSize]};
    mConversionSpecs.buf = mpBuffer.get();
  }

  void setType(const AdlibEmulator::Type type) { mPlayer.setType(type); }

  void render(Uint8* pOutBuffer, int bytesRequired)
  {
    auto pBuffer = mpBuffer.get();
    const auto samplesToRender = bytesRequired / mBytesPerSample;
    mPlayer.render(reinterpret_cast<std::int16_t*>(pBuffer), samplesToRender);

    mConversionSpecs.len = samplesToRender * sizeof(std::int16_t);
    SDL_ConvertAudio(&mConversionSpecs);
    std::memcpy(pOutBuffer, pBuffer, mConversionSpecs.len_cvt);
  }

  void playSong(data::Song&& song) { mPlayer.playSong(std::move(song)); }

  void setVolume(const float volume) { mPlayer.setVolume(volume); }

  SDL_AudioCVT mConversionSpecs;
  std::unique_ptr<std::uint8_t[]> mpBuffer;
  SoftwareImfPlayer mPlayer;
  int mBytesPerSample;
};


SoundSystem::LoadedSound::LoadedSound(RawBuffer buffer)
  : mData(std::move(buffer))
  , mpMixChunk(sdl_utils::wrap(
      Mix_QuickLoad_RAW(mData.data(), static_cast<Uint32>(mData.size()))))
{
}


SoundSystem::LoadedSound::LoadedSound(sdl_utils::Ptr<Mix_Chunk> pMixChunk)
  : mpMixChunk(std::move(pMixChunk))
{
}


SoundSystem::SoundSystem(
  const assets::ResourceLoader* pResources,
  const data::SoundStyle soundStyle,
  const data::AdlibPlaybackType adlibPlaybackType)
  : mCloseMixerGuard(std::invoke([]() {
    sdl_mixer::check(Mix_OpenAudio(
      DESIRED_SAMPLE_RATE,
      AUDIO_S16LSB,
      2, // stereo
      BUFFER_SIZE));

    return &Mix_CloseAudio;
  }))
  , mpResources(pResources)
  , mCurrentSoundStyle(soundStyle)
  , mCurrentAdlibPlaybackType(adlibPlaybackType)
{
  int sampleRate = 0;
  std::uint16_t audioFormat = 0;
  int numChannels = 0;
  Mix_QuerySpec(&sampleRate, &audioFormat, &numChannels);

  // Our music is in a format which SDL_mixer does not understand (IMF format
  // aka raw AdLib commands). Therefore, we cannot use any of the high-level
  // music playback functionality offered by the library. Instead, we register
  // our own callback handler and then use an AdLib emulator to generate audio
  // from the music data (SoftwareImfPlayer class).
  //
  // The SoftwareImfPlayer class only knows how to produce audio data in 16-bit
  // integer format (AUDIO_S16LSB), and in mono.  Converting from the player's
  // format into the output device format is handled by the ImfPlayerWrapper
  // class.
  mpMusicPlayer =
    std::make_unique<ImfPlayerWrapper>(audioFormat, sampleRate, numChannels);

  // For sound playback, we want to be able to play as many sound effects in
  // parallel as possible. In the original game, the number of available sound
  // effects is hardcoded into the executable, with sounds being identified by
  // a numerical index (sound ID). This allows us to implement a very simple
  // scheme: We allocate as many mixer channels as there are sound effects. We
  // then create one playable audio buffer (aka Mix_Chunk) for each sound
  // effect, and use its sound ID to determine which mixer channel it should be
  // played on. This way, all possible sound effects can play simultaneously,
  // but when the same sound effect is triggered multiple times in a row, it
  // results in the sound being cut off and played again from the beginning as
  // in the original game.
  Mix_AllocateChannels(data::NUM_SOUND_IDS);

  loadAllSounds(sampleRate, audioFormat, numChannels, soundStyle);

  setMusicVolume(data::MUSIC_VOLUME_DEFAULT);
  setSoundVolume(data::SOUND_VOLUME_DEFAULT);

  // Do this as the last step, in case any of the above throws an exception.
  // We would otherwise end up with a hook that points to a destroyed
  // SoundSystem instance, and crash.
  hookMusic();
}


SoundSystem::~SoundSystem()
{
  if (mpCurrentReplacementSong)
  {
    mpCurrentReplacementSong.reset();
  }
  else
  {
    unhookMusic();
  }
}


void SoundSystem::setSoundStyle(const data::SoundStyle soundStyle)
{
  if (soundStyle != mCurrentSoundStyle)
  {
    mCurrentSoundStyle = soundStyle;
    reloadAllSounds();
  }
}


void SoundSystem::setAdlibPlaybackType(
  const data::AdlibPlaybackType adlibPlaybackType)
{
  if (adlibPlaybackType == mCurrentAdlibPlaybackType)
  {
    return;
  }

  mCurrentAdlibPlaybackType = adlibPlaybackType;
  mpMusicPlayer->setType(toEmulationType(mCurrentAdlibPlaybackType));
  reloadAllSounds();
}


void SoundSystem::playSong(const std::string& name)
{
  if (auto pReplacementSong = loadReplacementSong(name))
  {
    mpCurrentReplacementSong = std::move(pReplacementSong);
    unhookMusic();
    Mix_PlayMusic(mpCurrentReplacementSong.get(), -1);
    return;
  }

  if (mpCurrentReplacementSong)
  {
    mpCurrentReplacementSong.reset();
    hookMusic();
  }

  mpMusicPlayer->playSong(mpResources->loadMusic(name));
}


void SoundSystem::stopMusic() const
{
  if (mpCurrentReplacementSong)
  {
    Mix_HaltMusic();
    mpCurrentReplacementSong.reset();
    hookMusic();
  }

  mpMusicPlayer->playSong({});
}


void SoundSystem::playSound(const data::SoundId id) const
{
  const auto index = idToIndex(id);
  Mix_PlayChannel(index, mSounds[index].mpMixChunk.get(), 0);
}


void SoundSystem::stopSound(const data::SoundId id) const
{
  const auto index = idToIndex(id);
  Mix_HaltChannel(index);
}


void SoundSystem::stopAllSounds() const
{
  data::forEachSoundId([&](const auto id) { stopSound(id); });
}


void SoundSystem::setMusicVolume(const float volume)
{
  const auto sdlVolume =
    static_cast<int>(std::clamp(volume, 0.0f, 1.0f) * MIX_MAX_VOLUME);

  mpMusicPlayer->setVolume(volume);
  Mix_VolumeMusic(sdlVolume);
}


void SoundSystem::setSoundVolume(const float volume)
{
  applySoundVolume(volume);
  mCurrentSoundVolume = volume;
}


void SoundSystem::loadAllSounds(
  const int sampleRate,
  const std::uint16_t audioFormat,
  const int numChannels,
  const data::SoundStyle soundStyle)
{
  const auto soundPackage = assets::loadAdlibSoundData(
    mpResources->file(assets::AUDIO_DICT_FILE),
    mpResources->file(assets::AUDIO_DATA_FILE));

  data::forEachSoundId([&](const auto id) {
    for (const auto& replacementPath : mpResources->replacementSoundPaths(id))
    {
      std::error_code ec;
      if (std::filesystem::exists(replacementPath, ec))
      {
        if (auto pMixChunk = Mix_LoadWAV(replacementPath.u8string().c_str()))
        {
          mSounds[idToIndex(id)] = LoadedSound{sdl_utils::wrap(pMixChunk)};
          return;
        }
      }
    }

    const auto soundData = loadSoundForStyle(
      id,
      soundStyle,
      sampleRate,
      *mpResources,
      soundPackage,
      toEmulationType(mCurrentAdlibPlaybackType));

    mSounds[idToIndex(id)] =
      LoadedSound{convertBuffer(soundData, audioFormat, numChannels)};
  });
}


void SoundSystem::reloadAllSounds()
{
  stopAllSounds();

  int sampleRate = 0;
  std::uint16_t audioFormat = 0;
  int numChannels = 0;
  Mix_QuerySpec(&sampleRate, &audioFormat, &numChannels);

  const auto soundPackage = assets::loadAdlibSoundData(
    mpResources->file(assets::AUDIO_DICT_FILE),
    mpResources->file(assets::AUDIO_DATA_FILE));

  data::forEachSoundId([&](const auto id) {
    const auto index = idToIndex(id);
    if (
      mSounds[index].mData.empty() || data::isIntroSound(id) ||
      !mpResources->hasSoundBlasterSound(id))
    {
      return;
    }

    const auto soundData = loadSoundForStyle(
      id,
      mCurrentSoundStyle,
      sampleRate,
      *mpResources,
      soundPackage,
      toEmulationType(mCurrentAdlibPlaybackType));
    mSounds[index] =
      LoadedSound{convertBuffer(soundData, audioFormat, numChannels)};
  });

  applySoundVolume(mCurrentSoundVolume);
}


void SoundSystem::applySoundVolume(const float volume)
{
  const auto sdlVolume =
    static_cast<int>(std::clamp(volume, 0.0f, 1.0f) * MIX_MAX_VOLUME);

  for (auto& sound : mSounds)
  {
    if (sound.mpMixChunk)
    {
      Mix_VolumeChunk(sound.mpMixChunk.get(), sdlVolume);
    }
  }
}


void SoundSystem::hookMusic() const
{
  Mix_HookMusic(
    [](void* pUserData, Uint8* pOutBuffer, int bytesRequired) {
      auto pWrapper = static_cast<ImfPlayerWrapper*>(pUserData);
      pWrapper->render(pOutBuffer, bytesRequired);
    },
    mpMusicPlayer.get());
}


void SoundSystem::unhookMusic() const
{
  Mix_HookMusic(nullptr, nullptr);
}


sdl_utils::Ptr<Mix_Music>
  SoundSystem::loadReplacementSong(const std::string& name)
{
  namespace fs = std::filesystem;

  if (const auto iCacheEntry = mReplacementSongFileCache.find(name);
      iCacheEntry != mReplacementSongFileCache.end())
  {
    if (iCacheEntry->second.empty())
    {
      // An empty entry indicates that no replacement exists
      return {};
    }

    if (auto pSong = Mix_LoadMUS(iCacheEntry->second.c_str()))
    {
      return sdl_utils::wrap(pSong);
    }
  }

  // Because of the large variety of file formats supported by SDL_mixer, we
  // don't try to explicitly look for specific file extensions. Instead, we
  // look for any file with a base name (i.e. without extension) matching the
  // requested music file's name. If we find a match and SDL_mixer can
  // successfully load it, we add the file path to our cache.
  const auto songName =
    fs::u8path(strings::toLowercase(name)).replace_extension();

  std::error_code ec;

  for (const auto& replacementBasePath :
       mpResources->replacementMusicBasePaths())
  {
    for (const fs::directory_entry& candidate :
         fs::directory_iterator(replacementBasePath, ec))
    {
      if (!candidate.is_regular_file() || candidate.path().stem() != songName)
      {
        continue;
      }

      const auto candidateFilePath = candidate.path().u8string();
      if (auto pSong = Mix_LoadMUS(candidateFilePath.c_str()))
      {
        auto pReplacement = sdl_utils::wrap(pSong);
        mReplacementSongFileCache.insert({name, candidateFilePath});
        return pReplacement;
      }
    }
  }

  // We didn't find a suitable replacement. Insert an empty string into the
  // cache to avoid scanning the file system again next time.
  mReplacementSongFileCache.insert({name, std::string{}});

  return {};
}

} // namespace rigel::audio
