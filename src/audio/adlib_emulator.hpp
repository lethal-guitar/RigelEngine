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

#include "base/math_tools.hpp"

#include <dbopl.h>
#include <opl3.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <variant>


namespace rigel::audio
{

constexpr auto OPL2_SAMPLE_RATE = 49716;

namespace detail
{

class DbOplAdlibEmulator
{
public:
  explicit DbOplAdlibEmulator(int sampleRate)
    : mEmulator(sampleRate)
  {
  }

  void writeRegister(const std::uint8_t reg, const std::uint8_t value)
  {
    mEmulator.WriteReg(reg, value);
  }

  template <typename OutputIt>
  void render(
    std::size_t numSamples,
    OutputIt destination,
    const float volumeScale = 1.0f)
  {
    // DBOPL outputs 32 bit samples, but they never exceed the 16 bit range
    // (compare source code comment in MixerChannel::AddSamples() in mixer.cpp
    // in the DosBox source). Still, this means we cannot render directly into
    // the output buffer.

    while (numSamples > 0)
    {
      const auto samplesForIteration = std::min(mTempBuffer.size(), numSamples);

      mEmulator.GenerateBlock2(
        static_cast<DBOPL::Bitu>(samplesForIteration), mTempBuffer.data());
      destination = std::transform(
        mTempBuffer.begin(),
        mTempBuffer.begin() + samplesForIteration,
        destination,
        [volumeScale](const auto sample32Bit) {
          return static_cast<std::int16_t>(
            std::clamp(base::round(sample32Bit * volumeScale), -16384, 16384));
        });

      numSamples -= samplesForIteration;
    }
  }

private:
  DBOPL::Chip mEmulator;
  std::array<std::int32_t, 256> mTempBuffer;
};


class NukedOpl3AdlibEmulator
{
public:
  explicit NukedOpl3AdlibEmulator(int sampleRate)
  {
    OPL3_Reset(&mEmulator, sampleRate);
  }

  NukedOpl3AdlibEmulator(const NukedOpl3AdlibEmulator&) = delete;
  NukedOpl3AdlibEmulator& operator=(const NukedOpl3AdlibEmulator&) = delete;

  void writeRegister(const std::uint8_t reg, const std::uint8_t value)
  {
    OPL3_WriteRegBuffered(&mEmulator, reg, value);
  }

  template <typename OutputIt>
  void render(
    std::size_t numSamples,
    OutputIt destination,
    const float volumeScale = 1.0f)
  {
    std::array<std::int16_t, 2> stereoPair;

    for (auto i = 0u; i < numSamples; ++i)
    {
      OPL3_GenerateResampled(&mEmulator, stereoPair.data());
      const auto mixedSample = stereoPair[0] * 0.5f + stereoPair[1] * 0.5f;
      *destination++ = base::roundTo<int16_t>(mixedSample * volumeScale);
    }
  }

private:
  opl3_chip mEmulator;
};

} // namespace detail


class AdlibEmulator
{
public:
  enum class Type
  {
    DBOPL,
    NukedOpl3
  };

  explicit AdlibEmulator(const int sampleRate, const Type type = Type::DBOPL)
    : mpEmulator([&]() {
      if (type == Type::NukedOpl3)
      {
        return std::make_unique<Emulator>(
          std::in_place_type<detail::NukedOpl3AdlibEmulator>, sampleRate);
      }

      return std::make_unique<Emulator>(
        std::in_place_type<detail::DbOplAdlibEmulator>, sampleRate);
    }())
  {
    // This is normally done by the game to select the right type of wave forms.
    // It's not part of the IMF files.
    writeRegister(1, 32);
  }

  void writeRegister(const std::uint8_t reg, const std::uint8_t value)
  {
    std::visit(
      [reg, value](auto&& emulator) { emulator.writeRegister(reg, value); },
      *mpEmulator);
  }

  template <typename OutputIt>
  void render(
    std::size_t numSamples,
    OutputIt destination,
    const float volumeScale = 1.0f)
  {
    std::visit(
      [&](auto&& emulator) {
        emulator.render(numSamples, destination, volumeScale);
      },
      *mpEmulator);
  }

  Type type() const
  {
    if (std::holds_alternative<detail::NukedOpl3AdlibEmulator>(*mpEmulator))
    {
      return Type::NukedOpl3;
    }

    return Type::DBOPL;
  }

private:
  using Emulator =
    std::variant<detail::DbOplAdlibEmulator, detail::NukedOpl3AdlibEmulator>;

  std::unique_ptr<Emulator> mpEmulator;
};

} // namespace rigel::audio
