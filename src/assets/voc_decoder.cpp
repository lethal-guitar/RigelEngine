/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 * Parts of this file are Copyright (C) 2002-2021  The DOSBox Team.
 * These parts are explicitly marked as such.
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

#include "voc_decoder.hpp"

#include "assets/file_utils.hpp"
#include "base/warnings.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>


/* Decoder for the Creative Voice File (VOC) format
 *
 * This is largely based on the information found at the following sources:
 *
 *  - http://www.shikadi.net/moddingwiki/VOC_Format
 *  - https://wiki.multimedia.cx/index.php?title=Creative_Voice
 *  - https://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM
 *
 * The ADPCM decoding has been adapted from DosBox code:
 * https://github.com/dosbox-staging/dosbox-staging/blob/65b5878b65267363bcb21d3a828854fe0a6ccbd8/src/hardware/sblaster.cpp
 */

namespace rigel::assets
{

using namespace std;

namespace
{

enum class ChunkType
{
  Terminator = 0,
  TypedSoundData = 1,
  UntypedSoundData = 2,
  Silence = 3,
  Marker = 4,
  Text = 5,
  RepeatStart = 6,
  RepeatEnd = 7,
  ExtendedParameters = 8,
  ExtendedTypedSoundData = 9
};


enum class CodecType
{
  Unsigned8BitPcm = 0,
  Adpcm4Bits = 1,
  Adpcm2_6Bits = 2,
  Adpcm2Bits = 3,
  Signed16BitPcm = 4,
};


enum class AdpcmType
{
  FourBits,
  TwoPointSixBits,
  TwoBits
};


ChunkType determineChunkType(const std::uint8_t typeMarker)
{
  if (typeMarker > 9)
  {
    throw std::invalid_argument("Unrecognized chunk type in VOC file");
  }

  return static_cast<ChunkType>(typeMarker);
}


CodecType determineCodecType(const std::uint8_t typeMarker)
{
  if (typeMarker > 4)
  {
    throw std::runtime_error("Unsupported codec in VOC file");
  }

  return static_cast<CodecType>(typeMarker);
}


int determineSampleRate(const std::uint8_t frequencyDivisor)
{
  return 1000000 / (256 - frequencyDivisor);
}


std::size_t calculateUncompressedSampleCount(
  const CodecType codec,
  const std::size_t encodedSize)
{
  switch (codec)
  {
    case CodecType::Unsigned8BitPcm:
    case CodecType::Signed16BitPcm:
      return encodedSize;

    // For the three ADPCM variants, each source byte decodes to N samples.
    // In addition, the first byte is a single Unsigned 8-bit sample.
    case CodecType::Adpcm4Bits:
      return 2 * (encodedSize - 1) + 1;

    case CodecType::Adpcm2_6Bits:
      return 3 * (encodedSize - 1) + 1;

    case CodecType::Adpcm2Bits:
      return 4 * (encodedSize - 1) + 1;

    default:
      break;
  }

  // We should not end up here, since unsupported codecs will throw an
  // exception earlier on.
  assert(false);
  return 0;
}


bool readAndValidateVocHeader(LeStreamReader& reader)
{
  const auto signatureText = readFixedSizeString(reader, 19);
  if (signatureText != "Creative Voice File")
  {
    return false;
  }

  const auto signatureByte = reader.readU8();
  if (signatureByte != 0x1A)
  {
    return false;
  }

  const auto headerSize = reader.readU16();
  if (headerSize != 0x1A)
  {
    return false;
  }

  const auto versionNumber = reader.readU16();
  const auto checkSum = reader.readU16();

  if (checkSum != (~versionNumber + 0x1234))
  {
    return false;
  }

  return true;
}


std::int16_t unsigned8BitSampleToSigned16Bit(const std::uint8_t sample)
{
  return 128 * (sample - 0x80);
}


// These lookup tables have been copied from DosBox code. Link to
// original code is next to each set of tables.
// The original code is Copyright (C) 2002-2021  The DOSBox Team
template <int numBits>
struct AdpcmLookupTables
{
};


// See
// https://github.com/dosbox-staging/dosbox-staging/blob/65b5878b65267363bcb21d3a828854fe0a6ccbd8/src/hardware/sblaster.cpp#L403
// The original code is Copyright (C) 2002-2021  The DOSBox Team
template <>
struct AdpcmLookupTables<2>
{
  static constexpr size_t TABLE_SIZE = 24;

  static constexpr std::array<int8_t, TABLE_SIZE> SAMPLE_SCALE_TABLE{
    0, 1,  0,  -1,  1, 3,  -1, -3,  2, 6,  -2,  -6,
    4, 12, -4, -12, 8, 24, -8, -24, 6, 48, -16, -48};
  static constexpr std::array<uint8_t, TABLE_SIZE> STEP_ADJUST_TABLE{
    0,   4, 0,   4, 252, 4, 252, 4, 252, 4, 252, 4,
    252, 4, 252, 4, 252, 4, 252, 4, 252, 0, 252, 0};
};


// See
// https://github.com/dosbox-staging/dosbox-staging/blob/65b5878b65267363bcb21d3a828854fe0a6ccbd8/src/hardware/sblaster.cpp#L426
// The original code is Copyright (C) 2002-2021  The DOSBox Team
template <>
struct AdpcmLookupTables<3>
{
  static constexpr size_t TABLE_SIZE = 40;

  static constexpr std::array<int8_t, TABLE_SIZE> SAMPLE_SCALE_TABLE{
    0,  1,   2,   3,   0,  -1, -2, -3, 1,   3,   5,   7,  -1, -3,
    -5, -7,  2,   6,   10, 14, -2, -6, -10, -14, 4,   12, 20, 28,
    -4, -12, -20, -28, 5,  15, 25, 35, -5,  -15, -25, -35};

  static constexpr std::array<uint8_t, TABLE_SIZE> STEP_ADJUST_TABLE{
    0,   0, 0, 8, 0,   0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8,
    248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 0, 248, 0, 0, 0};
};


// See
// https://github.com/dosbox-staging/dosbox-staging/blob/65b5878b65267363bcb21d3a828854fe0a6ccbd8/src/hardware/sblaster.cpp#L375
// The original code is Copyright (C) 2002-2021  The DOSBox Team
template <>
struct AdpcmLookupTables<4>
{
  static constexpr size_t TABLE_SIZE = 64;

  static constexpr std::array<int8_t, TABLE_SIZE> SAMPLE_SCALE_TABLE{
    0, 1,  2,  3,  4,  5,  6,  7,  0,  -1,  -2,  -3,  -4,  -5,  -6,  -7,
    1, 3,  5,  7,  9,  11, 13, 15, -1, -3,  -5,  -7,  -9,  -11, -13, -15,
    2, 6,  10, 14, 18, 22, 26, 30, -2, -6,  -10, -14, -18, -22, -26, -30,
    4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60};

  static constexpr std::array<uint8_t, TABLE_SIZE> STEP_ADJUST_TABLE{
    0,   0, 0, 0, 0, 16, 16, 16, 0,   0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 0,  0,  0,  240, 0, 0, 0, 0, 0,  0,  0};
};


class AdpcmDecoderHelper
{
public:
  explicit AdpcmDecoderHelper(const uint8_t initialSample)
    : mReference(initialSample)
    , mStepSize(0)
  {
  }

  template <int numBits>
  int16_t decodeBits(const int encodedSample)
  {
    // This algorithm has been adapted from DosBox code.
    // See
    // https://github.com/dosbox-staging/dosbox-staging/blob/65b5878b65267363bcb21d3a828854fe0a6ccbd8/src/hardware/sblaster.cpp#L391
    // The original code is Copyright (C) 2002-2021  The DOSBox Team
    const auto tableIndex = std::min(
      encodedSample + mStepSize,
      int(AdpcmLookupTables<numBits>::TABLE_SIZE - 1));

    const auto newStepSize =
      mStepSize + AdpcmLookupTables<numBits>::STEP_ADJUST_TABLE[tableIndex];
    mStepSize = newStepSize & 0xff;

    const auto newSample =
      mReference + AdpcmLookupTables<numBits>::SAMPLE_SCALE_TABLE[tableIndex];
    mReference = static_cast<uint8_t>(std::clamp(newSample, 0, 255));

    return unsigned8BitSampleToSigned16Bit(mReference);
  }

private:
  uint8_t mReference;
  int mStepSize;
};


template <typename TargetIter>
void decodeAdpcmAudio(
  LeStreamReader& reader,
  const AdpcmType codec,
  const std::size_t encodedSize,
  TargetIter outputIter)
{
  const auto firstSample = reader.readU8();
  *outputIter++ = unsigned8BitSampleToSigned16Bit(firstSample);

  AdpcmDecoderHelper decoder(firstSample);
  for (auto i = 0u; i < encodedSize - 1; ++i)
  {
    const auto bitPack = reader.readU8();

    switch (codec)
    {
      case AdpcmType::FourBits:
        // Each byte contains two 4-bit encoded samples
        *outputIter++ = decoder.template decodeBits<4>(bitPack >> 4);
        *outputIter++ = decoder.template decodeBits<4>(bitPack & 0x0F);
        break;

      case AdpcmType::TwoPointSixBits:
        // Each byte contains two 3-bit samples and one 2-bit sample
        *outputIter++ = decoder.template decodeBits<3>((bitPack >> 5) & 0x07);
        *outputIter++ = decoder.template decodeBits<3>((bitPack >> 2) & 0x07);
        *outputIter++ = decoder.template decodeBits<3>((bitPack & 0x03) << 1);
        break;

      case AdpcmType::TwoBits:
        // Each byte contains four 2-bit encoded samples
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 6) & 0x03);
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 4) & 0x03);
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 2) & 0x03);
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 0) & 0x03);
        break;
    }
  }
}


template <typename TargetIter>
void decodeAudio(
  LeStreamReader& reader,
  const std::size_t encodedSize,
  const CodecType codec,
  TargetIter outputIter)
{
  switch (codec)
  {
    case CodecType::Unsigned8BitPcm:
      for (auto i = 0u; i < encodedSize; ++i)
      {
        *outputIter++ = unsigned8BitSampleToSigned16Bit(reader.readU8());
      }
      break;

    case CodecType::Adpcm4Bits:
      decodeAdpcmAudio(reader, AdpcmType::FourBits, encodedSize, outputIter);
      break;

    case CodecType::Adpcm2_6Bits:
      decodeAdpcmAudio(
        reader, AdpcmType::TwoPointSixBits, encodedSize, outputIter);
      break;

    case CodecType::Adpcm2Bits:
      decodeAdpcmAudio(reader, AdpcmType::TwoBits, encodedSize, outputIter);
      break;

    case CodecType::Signed16BitPcm:
      for (auto i = 0u; i < encodedSize; ++i)
      {
        *outputIter++ = reader.readS16();
      }
      break;
  }
}

} // namespace


data::AudioBuffer decodeVoc(const ByteBuffer& data)
{
  LeStreamReader reader(data);
  if (!readAndValidateVocHeader(reader))
  {
    throw std::invalid_argument("Invalid VOC file header");
  }

  std::vector<data::Sample> decodedSamples;
  std::optional<int> sampleRate;

  while (reader.hasData())
  {
    const auto chunkType = determineChunkType(reader.readU8());
    if (chunkType == ChunkType::Terminator)
    {
      // Terminator chunks don't have a size value, so we need to stop before
      // attempting to read a size.
      break;
    }
    const auto chunkSize = reader.readU24();

    LeStreamReader chunkReader(
      reader.currentIter(), reader.currentIter() + chunkSize);

    switch (chunkType)
    {
      case ChunkType::TypedSoundData:
        {
          const auto newSampleRate = determineSampleRate(chunkReader.readU8());
          if (sampleRate && *sampleRate != newSampleRate)
          {
            throw std::invalid_argument(
              "Multiple sample rates in single VOC file aren't supported");
          }
          else if (!sampleRate)
          {
            sampleRate = newSampleRate;
          }

          const auto codecType = determineCodecType(chunkReader.readU8());
          const auto encodedAudioSize = chunkSize - sizeof(std::uint8_t) * 2;
          decodedSamples.reserve(
            decodedSamples.size() +
            calculateUncompressedSampleCount(codecType, encodedAudioSize));
          decodeAudio(
            chunkReader,
            encodedAudioSize,
            codecType,
            std::back_inserter(decodedSamples));
        }
        break;

      case ChunkType::Silence:
        {
          auto numSilentSamples = chunkReader.readU16() + 1u;
          const auto silenceSampleRate =
            determineSampleRate(chunkReader.readU8());

          if (sampleRate && *sampleRate != silenceSampleRate)
          {
            const auto factor =
              static_cast<double>(*sampleRate) / silenceSampleRate;
            numSilentSamples = static_cast<decltype(numSilentSamples)>(
              numSilentSamples * factor);
          }
          else if (!sampleRate)
          {
            sampleRate = silenceSampleRate;
          }

          decodedSamples.resize(decodedSamples.size() + numSilentSamples, 0);
        }
        break;

      case ChunkType::UntypedSoundData:
      case ChunkType::ExtendedParameters:
      case ChunkType::ExtendedTypedSoundData:
        throw std::runtime_error("VOC file chunk type not supported");

      default:
        // Marker, text, and repeat chunks will just be skipped over.
        break;
    }

    reader.skipBytes(chunkSize);
  }

  if (!sampleRate || decodedSamples.empty())
  {
    throw std::invalid_argument("VOC file didn't contain data");
  }

  return {*sampleRate, std::move(decodedSamples)};
}


} // namespace rigel::assets
