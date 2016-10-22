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

#include <sdl_utils/error.hpp>
#include <sdl_utils/ptr.hpp>
#include <utility>
#include <utils/math_tools.hpp>

#include <speex/speex_resampler.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
  using ResamplerPtr =
    std::unique_ptr<SpeexResamplerState, decltype(&speex_resampler_destroy)>;
  ResamplerPtr pResampler(
    speex_resampler_init(
      1,
      buffer.mSampleRate,
      newSampleRate,
      5,
      nullptr),
    &speex_resampler_destroy);
  speex_resampler_skip_zeros(pResampler.get());

  auto inputLength = static_cast<spx_uint32_t>(buffer.mSamples.size());
  auto outputLength = static_cast<spx_uint32_t>(
    utils::integerDivCeil<spx_uint32_t>(
      inputLength, buffer.mSampleRate) * newSampleRate);

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


void appendRampToZero(data::AudioBuffer& buffer) {
  // Roughly 10 ms of linear ramp
  const auto rampLength = (SAMPLE_RATE / 100);

  buffer.mSamples.reserve(buffer.mSamples.size() + rampLength - 1);
  auto lastSample = buffer.mSamples.back();

  for (int i=1; i<rampLength; ++i) {
    const auto interpolation = i / static_cast<double>(rampLength);
    const auto rampedValue = lastSample * (1.0 - interpolation);
    buffer.mSamples.push_back(static_cast<data::Sample>(
      std::round(rampedValue)));
  }
}

}


struct SoundSystem::SoundSystemImpl
{
  std::unordered_map<SoundHandle, sdl_utils::Ptr<Mix_Chunk>> mLoadedChunks;
  std::vector<data::AudioBuffer> mAudioBuffers;
  SoundHandle mNextHandle = 0;
  mutable int mCurrentSongChannel = -1;
  mutable int mCurrentSoundChannel = -1;


  SoundSystemImpl() {
    sdl_utils::throwIfFailed([]() {
      return Mix_OpenAudio(
        SAMPLE_RATE,
        MIX_DEFAULT_FORMAT,
        1, // stereo
        BUFFER_SIZE);
    });
  }


  ~SoundSystemImpl() {
    Mix_Quit();
  }


  void reportMemoryUsage() const {
    using namespace std;

    size_t totalUsedMemory =
      sizeof(SoundSystemImpl);

    for (const auto& buffer : mAudioBuffers) {
      totalUsedMemory += sizeof(data::AudioBuffer);
      totalUsedMemory += sizeof(data::Sample) * buffer.mSamples.size();
    }

    totalUsedMemory +=
      (sizeof(Mix_Chunk) + sizeof(SoundHandle)) * mLoadedChunks.size();

    string suffix("B");
    if (totalUsedMemory >= 1024) {
      suffix = "KB";
      totalUsedMemory /= 1024;
    }

    if (totalUsedMemory >= 1024) {
      suffix = "MB";
      totalUsedMemory /= 1024;
    }

    cout << "SoundSystem memory usage: " << totalUsedMemory << suffix << '\n';
  }


  SoundHandle addSound(
    const data::AudioBuffer& original
  ) {
    auto buffer = resampleAudio(original, SAMPLE_RATE);
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
    return addSong(convertedBuffer);
#else
    return addSong(buffer);
#endif
  }


  SoundHandle addSong(
    data::AudioBuffer buffer
  ) {
    mAudioBuffers.emplace_back(buffer);
    const auto assignedHandle = mNextHandle++;
    mLoadedChunks.emplace(
      assignedHandle,
      createMixChunk(mAudioBuffers.back()));
    return assignedHandle;
  }


  void playSong(const SoundHandle handle) const {
    const auto it = mLoadedChunks.find(handle);
    assert(it != mLoadedChunks.end());

    stopMusic();
    mCurrentSongChannel = Mix_PlayChannel(0, it->second.get(), -1);
  }


  void stopMusic() const {
    if (mCurrentSongChannel != -1) {
      Mix_HaltChannel(mCurrentSongChannel);
    }
  }

  void playSound(
    const SoundHandle handle,
    float volume,
    float pan
  ) {
    const auto it = mLoadedChunks.find(handle);
    assert(it != mLoadedChunks.end());
    Mix_PlayChannel(1, it->second.get(), 0);
  }
};


SoundSystem::SoundSystem()
  : mpImpl(std::make_unique<SoundSystemImpl>())
{
}

SoundSystem::~SoundSystem() = default;

void SoundSystem::reportMemoryUsage() const {
  mpImpl->reportMemoryUsage();
}

SoundHandle SoundSystem::addSong(data::AudioBuffer buffer) {
  return mpImpl->addSong(std::move(buffer));
}

SoundHandle SoundSystem::addSound(data::AudioBuffer buffer) {
 return  mpImpl->addSound(std::move(buffer));
}

void SoundSystem::playSong(SoundHandle handle) const {
  mpImpl->playSong(handle);
}

void SoundSystem::stopMusic() const {
  mpImpl->stopMusic();
}

void SoundSystem::playSound(
  SoundHandle handle,
  float volume,
  float pan
) const {
  mpImpl->playSound(handle, volume, pan);
}

}}
