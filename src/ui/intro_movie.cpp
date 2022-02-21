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

#include "intro_movie.hpp"

#include "assets/resource_loader.hpp"
#include "frontend/game_service_provider.hpp"


// Repetition counts and delays from original exe, determined from disassembly:
//
// | M  | Rep | Delay |
// | F5 | N/A |    35 | // Repeats forever, length controlled by music playback
// |    |     |    60 | // Additional delay between Logo and intro movies
// | F2 |   6 |    70 |
// | F1 |  10 |    14 |
// | F3 |   2 |    23 |
// | F4 |   1 |   N/A | // delay changes during playback, see below
//
// Sound triggers:
//
//  F1: Play INTRO3 on first frame of each repetition
//  F3: Play INTRO4 on frames 0, 3 and 6 of each repetition
//
//
// F4 Delays and sound triggers by frame:
//
// |  F |  Sound | Delay |
// -----------------------
// |  0 | INTRO5 |    46 |
// |  7 | INTRO6 |    46 |
// | 17 |        |    46 |
// | 23 |        |   560 | // 2 seconds
// | 24 |        |    46 |
// | 31 | INTRO7 |   560 | // 2 seconds
// | 32 |        |   280 | // 1 second
// | 33 | INTRO8 |    56 |
// | 37 | INTRO9 |       |
// | 39 |        |   280 | // 1 second
// | 40 |        |    16 |
// | 49 |   SB_1 |   280 | // 1 second
// | 50 |        |    16 |
// | 55 |   SB_1 |  1120 | // 4 seconds


namespace rigel::ui
{

using data::SoundId;


IntroMovie::PlaybackConfigList
  IntroMovie::createConfigurations(const assets::ResourceLoader& resources)
{
  // clang-format off
  return {
    // Neo LA - the future
    {
      resources.loadMovie("NUKEM2.F2"),
      70,
      6,
      nullptr
    },

    // Focus on Duke shooting at range
    {
      resources.loadMovie("NUKEM2.F1"),
      14,
      10,
      [pServiceProvider = mpServiceProvider](const int frame) {
        if (frame == 0) {
          pServiceProvider->playSound(SoundId::IntroGunShot);
        }

        return std::nullopt;
      }
    },

    // Focus on target being hit
    {
      resources.loadMovie("NUKEM2.F3"),
      23,
      2,
      [pServiceProvider = mpServiceProvider](const int frame) {
        if (frame == 0 || frame == 3 || frame == 6) {
          pServiceProvider->playSound(SoundId::IntroGunShotLow);
        }

        return std::nullopt;
      }
    },

    // Remainder of shooting range scene
    {
      resources.loadMovie("NUKEM2.F4"),
      46,
      1,
      [pServiceProvider = mpServiceProvider](const int frame) {
        std::optional<int> newFrameDelay = std::nullopt;

        switch (frame) {
          case 0:
            pServiceProvider->playSound(SoundId::IntroEmptyShellsFalling);
            break;

          case 7:
            pServiceProvider->playSound(SoundId::IntroTargetMovingCloser);
            break;

          case 23:
            // 2 second freeze frame on smiling Duke
            newFrameDelay = 560;
            break;

          case 24:
            newFrameDelay = 46;
            break;

          case 31:
            // 2 second freeze frame on target (now up close)
            pServiceProvider->stopSound(SoundId::IntroTargetMovingCloser);
            pServiceProvider->playSound(SoundId::IntroTargetStopsMoving);
            newFrameDelay = 560;
            break;

          case 32:
            // 1 second freeze frame on Duke looking at target
            newFrameDelay = 280;
            break;

          case 33:
            pServiceProvider->playSound(SoundId::IntroDukeSpeaks1);
            newFrameDelay = 56;
            break;

          case 37:
            pServiceProvider->playSound(SoundId::IntroDukeSpeaks2);
            break;

          case 39:
            // 1 second freeze frame on Duke after he spoke
            newFrameDelay = 280;
            break;

          case 40:
            // Begin logo text slide in
            newFrameDelay = 16;
            break;

          case 49:
            // 1st logo text smash, 1 second freeze
            pServiceProvider->playSound(SoundId::BigExplosion);
            newFrameDelay = 280;
            break;

          case 50:
            // Begin 2nd phase logo slide in (letters "II")
            newFrameDelay = 16;
            break;

          case 55:
            // Show logo for 4 seconds
            pServiceProvider->playSound(SoundId::BigExplosion);
            newFrameDelay = 1120;
            break;
        }

        return newFrameDelay;
      }
    }
  };
  // clang-format on
}


IntroMovie::IntroMovie(GameMode::Context context)
  : mpServiceProvider(context.mpServiceProvider)
  , mMoviePlayer(context.mpRenderer)
  , mCurrentConfiguration(0u)
{
  mMovieConfigurations = createConfigurations(*context.mpResources);
}


void IntroMovie::start()
{
  mpServiceProvider->playMusic("RANGEA.IMF");
  mCurrentConfiguration = 0u;
  startNextMovie();
}


void IntroMovie::updateAndRender(engine::TimeDelta dt)
{
  if (isFinished())
  {
    return;
  }

  mMoviePlayer.updateAndRender(dt);

  if (mMoviePlayer.hasCompletedPlayback())
  {
    ++mCurrentConfiguration;
    if (isFinished())
    {
      mpServiceProvider->fadeOutScreen();
      return;
    }

    startNextMovie();
  }
}


bool IntroMovie::isFinished() const
{
  return mCurrentConfiguration >= mMovieConfigurations.size();
}


void IntroMovie::startNextMovie()
{
  const auto& config = mMovieConfigurations[mCurrentConfiguration];
  mMoviePlayer.playMovie(
    config.mMovie,
    config.mFrameDelay,
    config.mRepetitions,
    config.mFrameCallback);
}

} // namespace rigel::ui
