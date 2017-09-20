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

#include "base/math_tools.hpp"
#include "engine/imf_player.hpp"
#include "sdl_utils/error.hpp"

#include <cassert>
#include <cmath>
#include <utility>

namespace rigel { namespace engine {

namespace {

using SoundHandle = SoundSystem::SoundHandle;

const auto SAMPLE_RATE = 44100;
const auto BUFFER_SIZE = 2048;


sdl_utils::Ptr<Mix_Chunk> createMixChunk(data::AudioBuffer& buffer) {
  const auto bufferSize = buffer.mSamples.size() * sizeof(data::Sample);
  return sdl_utils::Ptr<Mix_Chunk>(
    Mix_QuickLoad_RAW(reinterpret_cast<Uint8*>(buffer.mSamples.data()),
    static_cast<Uint32>(bufferSize)));
}


data::AudioBuffer resampleAudio(
  const data::AudioBuffer& buffer,
  const int newSampleRate
) {
  const auto ratio = static_cast<float>(buffer.mSampleRate) / newSampleRate;

  const auto inputLength = buffer.mSamples.size();
  const auto outputLength = static_cast<std::size_t>(
    std::round(inputLength / ratio));

  std::vector<data::Sample> resampled(outputLength);
  for (std::size_t i = 0; i < outputLength; ++i) {
    const auto sourceIndexFloat = i * ratio;

    const auto sourceIndex1 = static_cast<int>(std::floor(sourceIndexFloat));
    const auto sourceIndex2 = static_cast<int>(std::ceil(sourceIndexFloat));

    const auto positionBetweenSamples = sourceIndexFloat - sourceIndex1;

    const auto interpolated = base::lerp(
      buffer.mSamples[sourceIndex1],
      buffer.mSamples[sourceIndex2],
      positionBetweenSamples);

    resampled[i] = static_cast<data::Sample>(std::round(interpolated));
  }
  return {newSampleRate, resampled};
}


void appendRampToZero(data::AudioBuffer& buffer) {
  // Roughly 10 ms of linear ramp
  const auto rampLength = (SAMPLE_RATE / 100);

  buffer.mSamples.reserve(buffer.mSamples.size() + rampLength - 1);
  const auto lastSample = buffer.mSamples.back();

  for (int i=1; i<rampLength; ++i) {
    const auto interpolation = i / static_cast<double>(rampLength);
    const auto rampedValue = lastSample * (1.0 - interpolation);
    buffer.mSamples.push_back(static_cast<data::Sample>(
      std::round(rampedValue)));
  }
}

}


SoundSystem::SoundSystem()
  : mpMusicPlayer(std::make_unique<ImfPlayer>(SAMPLE_RATE))
{
  sdl_utils::throwIfFailed([]() {
    return Mix_OpenAudio(
      SAMPLE_RATE,
      MIX_DEFAULT_FORMAT,
      1, // mono
      BUFFER_SIZE);
  });
  Mix_HookMusic(
    [](void* pUserData, Uint8* pOutBuffer, int bytesRequired) {
      auto pPlayer = static_cast<ImfPlayer*>(pUserData);
      auto pDestination = reinterpret_cast<std::int16_t*>(pOutBuffer);
      const auto samplesRequired = bytesRequired / sizeof(std::int16_t);

      pPlayer->render(pDestination, samplesRequired);
    },
    mpMusicPlayer.get());
}


SoundSystem::~SoundSystem() {
  Mix_HookMusic(nullptr, nullptr);
  mLoadedChunks.clear();
  Mix_Quit();
}


SoundHandle SoundSystem::addSound(const data::AudioBuffer& original) {
  auto buffer = original.mSampleRate == SAMPLE_RATE
    ? original
    : resampleAudio(original, SAMPLE_RATE);
  if (buffer.mSamples.back() != 0) {
    // Prevent clicks/pops with samples that don't return to 0 at the end
    // by adding a small linear ramp leading back to zero.
    appendRampToZero(buffer);
  }

#if MIX_DEFAULT_FORMAT != AUDIO_S16LSB
  SDL_AudioSpec originalSoundSpec{
    buffer.mSampleRate,
    AUDIO_S16LSB,
    1,
    0, 0, 0, 0, nullptr};
  SDL_AudioCVT conversionSpecs;
  SDL_BuildAudioCVT(
    &conversionSpecs,
    AUDIO_S16LSB, 1, buffer.mSampleRate,
    MIX_DEFAULT_FORMAT, 1, SAMPLE_RATE);

  conversionSpecs.len = static_cast<int>(
    buffer.mSamples.size() * 2);
  std::vector<data::Sample> tempBuffer(
    conversionSpecs.len * conversionSpecs.len_mult);
  conversionSpecs.buf = reinterpret_cast<Uint8*>(tempBuffer.data());
  std::copy(
    buffer.mSamples.cbegin(), buffer.mSamples.cend(), tempBuffer.begin());

  SDL_ConvertAudio(&conversionSpecs);

  data::AudioBuffer convertedBuffer{SAMPLE_RATE};
  convertedBuffer.mSamples.insert(
    convertedBuffer.mSamples.end(),
    tempBuffer.cbegin(),
    tempBuffer.cbegin() + conversionSpecs.len_cvt);
  return addConvertedSound(convertedBuffer);
#else
  return addConvertedSound(buffer);
#endif
}


SoundHandle SoundSystem::addConvertedSound(data::AudioBuffer buffer) {
  mAudioBuffers.emplace_back(std::move(buffer));
  const auto assignedHandle = mNextHandle++;
  mLoadedChunks.emplace(
    assignedHandle,
    createMixChunk(mAudioBuffers.back()));
  return assignedHandle;
}


void SoundSystem::playSong(data::Song&& song) {
  mpMusicPlayer->playSong(std::move(song));
}


void SoundSystem::stopMusic() const {
  mpMusicPlayer->playSong({});
}


void SoundSystem::playSound(const SoundHandle handle) const {
  const auto it = mLoadedChunks.find(handle);
  assert(it != mLoadedChunks.end());
  Mix_PlayChannel(1, it->second.get(), 0);
}

}}
