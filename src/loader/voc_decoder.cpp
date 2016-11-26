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

#include "voc_decoder.hpp"

#include "base/warnings.hpp"
#include "loader/file_utils.hpp"
#include "utils/math_tools.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <iterator>


/* Decoder for the Creative Voice File (VOC) format
 *
 * This is largely based on the information found at the following sources:
 *
 *  - http://www.shikadi.net/moddingwiki/VOC_Format
 *  - https://wiki.multimedia.cx/index.php?title=Creative_Voice
 *  - https://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM
 *
 * The ADPCM decoding is also heavily inspired by FFMPEG's implementation of
 * the same, which can be found in libavcodec/adpcm.c:
 *
 *   https://www.ffmpeg.org/doxygen/2.4/adpcm_8c_source.html#l00295
 */

namespace rigel { namespace loader {

using namespace std;

namespace {

enum class ChunkType {
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


enum class CodecType {
  Unsigned8BitPcm = 0,
  Adpcm4Bits = 1,
  Adpcm2_6Bits = 2,
  Adpcm2Bits = 3,
  Signed16BitPcm = 4,
};


enum class AdpcmType {
  FourBits,
  TwoPointSixBits,
  TwoBits
};


constexpr int adpcmShiftValue(const AdpcmType codec) {
  return codec == AdpcmType::TwoBits ? 2 : 0;
}


ChunkType determineChunkType(const std::uint8_t typeMarker) {
  if (typeMarker > 9) {
    throw std::invalid_argument("Unrecognized chunk type in VOC file");
  }

  return static_cast<ChunkType>(typeMarker);
}


CodecType determineCodecType(const std::uint8_t typeMarker) {
  if (typeMarker > 4) {
    throw std::runtime_error("Unsupported codec in VOC file");
  }

  return static_cast<CodecType>(typeMarker);
}


int determineSampleRate(const std::uint8_t frequencyDivisor) {
  return 1000000 / (256 - frequencyDivisor);
}


std::size_t calculateUncompressedSampleCount(
  const CodecType codec,
  const std::size_t encodedSize
) {
  switch (codec) {
    case CodecType::Unsigned8BitPcm:
    case CodecType::Signed16BitPcm:
      return encodedSize;

    // For the three ADPCM variants, each source byte decodes to N samples.
    // In addition, the first byte is a single Unsigned 8-bit sample.
    case CodecType::Adpcm4Bits:
      return 2 * (encodedSize-1) + 1;

    case CodecType::Adpcm2_6Bits:
      return 3 * (encodedSize-1) + 1;

    case CodecType::Adpcm2Bits:
      return 4  * (encodedSize-1) + 1;

    default:
      break;
  }

  // We should not end up here, since unsupported codecs will throw an
  // exception earlier on.
  assert(false);
  return 0;
}


bool readAndValidateVocHeader(LeStreamReader& reader) {
  const auto signatureText = readFixedSizeString(reader, 19);
  if (signatureText != "Creative Voice File") {
    return false;
  }

  const auto signatureByte = reader.readU8();
  if (signatureByte != 0x1A) {
    return false;
  }

  const auto headerSize = reader.readU16();
  if (headerSize != 0x1A) {
    return false;
  }

  const auto versionNumber = reader.readU16();
  const auto checkSum = reader.readU16();

  if (checkSum != (~versionNumber + 0x1234)) {
    return false;
  }

  return true;
}


std::int16_t unsigned8BitSampleToSigned16Bit(const std::uint8_t sample) {
  return 128 * (sample - 0x80);
}


template<AdpcmType codec>
class AdpcmDecoderHelper {
public:
  static const int shift = adpcmShiftValue(codec);

  explicit AdpcmDecoderHelper(const int32_t initialPrediction)
    : mPrediction(initialPrediction)
    , mStep(0)
  {
  }

  template<int numBits>
  int16_t decodeBits(const int bitPack) {
    const bool isNegative = (bitPack >> (numBits-1)) != 0;
    const int delta = bitPack & ((1 << (numBits-1)) - 1);

    int difference = delta << (mStep + 7 + shift);
    if (isNegative) {
      difference = -difference;
    }

    const auto newSample = utils::clamp(
      mPrediction + difference, -16384, 16384);
    mPrediction = newSample;

    const auto limit = numBits*2 - 3;
    if (delta >= limit && mStep < 3) {
      ++mStep;
    } else if (delta == 0 && mStep > 0) {
      --mStep;
    }

    return static_cast<int16_t>(newSample);
  }

private:
  int32_t mPrediction;
  int mStep;
};


template<AdpcmType codec, typename TargetIter>
void decodeAdpcmAudio(
  LeStreamReader& reader,
  const std::size_t encodedSize,
  TargetIter outputIter
) {
  const auto firstSample = unsigned8BitSampleToSigned16Bit(reader.readU8());
  *outputIter++ = firstSample;

  AdpcmDecoderHelper<codec> decoder(firstSample);
  for (auto i=0u; i<encodedSize - 1; ++i) {
    const auto bitPack = reader.readU8();

    switch (codec) {
      case AdpcmType::FourBits:
        // Each byte contains two 4-bit encoded samples
        *outputIter++ = decoder.template decodeBits<4>(bitPack >> 4);
        *outputIter++ = decoder.template decodeBits<4>(bitPack & 0x0F);
        break;

      case AdpcmType::TwoPointSixBits:
        // Each byte contains two 3-bit samples and one 2-bit sample
        *outputIter++ = decoder.template decodeBits<3>(bitPack >> 5);
        *outputIter++ = decoder.template decodeBits<3>((bitPack >> 2) & 0x07);
        *outputIter++ = decoder.template decodeBits<2>(bitPack & 0x03);
        break;

      case AdpcmType::TwoBits:
        // Each byte contains four 2-bit encoded samples
        *outputIter++ = decoder.template decodeBits<2>(bitPack >> 6);
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 4) & 0x03);
        *outputIter++ = decoder.template decodeBits<2>((bitPack >> 2) & 0x03);
        *outputIter++ = decoder.template decodeBits<2>(bitPack & 0x03);
        break;
    }
  }
}


template<typename TargetIter>
void decodeAudio(
  LeStreamReader& reader,
  const std::size_t encodedSize,
  const CodecType codec,
  TargetIter outputIter
) {
  switch (codec) {
    case CodecType::Unsigned8BitPcm:
      for (auto i=0u; i<encodedSize; ++i) {
        *outputIter++ = unsigned8BitSampleToSigned16Bit(reader.readU8());
      }
      break;

    case CodecType::Adpcm4Bits:
      decodeAdpcmAudio<AdpcmType::FourBits>(reader, encodedSize, outputIter);
      break;

    case CodecType::Adpcm2_6Bits:
      decodeAdpcmAudio<AdpcmType::TwoPointSixBits>(
        reader, encodedSize, outputIter);
      break;

    case CodecType::Adpcm2Bits:
      decodeAdpcmAudio<AdpcmType::TwoBits>(reader, encodedSize, outputIter);
      break;

    case CodecType::Signed16BitPcm:
      for (auto i=0u; i<encodedSize; ++i) {
        *outputIter++ = reader.readS16();
      }
      break;
  }
}

}


data::AudioBuffer decodeVoc(const ByteBuffer& data) {
  LeStreamReader reader(data);
  if (!readAndValidateVocHeader(reader)) {
    throw std::invalid_argument("Invalid VOC file header");
  }

  std::vector<data::Sample> decodedSamples;
  boost::optional<int> sampleRate;

  while (reader.hasData()) {
    const auto chunkType = determineChunkType(reader.readU8());
    if (chunkType == ChunkType::Terminator) {
      // Terminator chunks don't have a size value, so we need to stop before
      // attempting to read a size.
      break;
    }
    const auto chunkSize = reader.readU24();

    LeStreamReader chunkReader(
      reader.currentIter(),
      reader.currentIter() + chunkSize);

    switch (chunkType) {
      case ChunkType::TypedSoundData:
        {
          const auto newSampleRate = determineSampleRate(chunkReader.readU8());
          if (sampleRate && *sampleRate != newSampleRate) {
            throw std::invalid_argument(
              "Multiple sample rates in single VOC file aren't supported");
          } else if (!sampleRate) {
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

          if (sampleRate && *sampleRate != silenceSampleRate) {
            const auto factor =
              static_cast<double>(*sampleRate) / silenceSampleRate;
            numSilentSamples = static_cast<decltype(numSilentSamples)>(
              numSilentSamples * factor);
          } else if (!sampleRate) {
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

  if (!sampleRate || decodedSamples.empty()) {
    throw std::invalid_argument("VOC file didn't contain data");
  }

  return {*sampleRate, std::move(decodedSamples)};
}


}}
