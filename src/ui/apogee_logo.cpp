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

#include "apogee_logo.hpp"

#include "common/game_service_provider.hpp"
#include "loader/resource_loader.hpp"


namespace rigel { namespace ui {

namespace {

/* The original Duke2 executable contains a weird hack for the Apogee Logo.
 * Instead of specifying a duration for the video playback, as is the case with
 * the other intro videos, the length is derived from the length of the song
 * that plays during the video. More specifically, the video playback is
 * stopped once there are 40 bytes (or less) of data left to process in the
 * FANFAREA.IMF file. The now frozen video image is displayed for 60 more
 * ticks before the next stage (intro movie) is entered.
 * This gives the music a bit more time to play, but not enough to play to
 * completion - the song in its entirety is 2093 ticks long (~ 7.5 seconds),
 * but only 1721 ticks are played (~ 6.15 seconds).
 *
 * Funnily enough, that specific code does not run if music playback is
 * disabled, which has the effect of indefinitely looping the Apogee Logo
 * animation if music is turned off - until a key is pressed. I'm not sure if
 * that is a bug or a feature, but I suspect the former ;)
 *
 * Technically, in order to behave exactly as the original game would in the
 * presence of mods, we would need to parse the contents of FANFAREA.IMF and
 * determine how much ticks would have elapsed by the point where we reach the
 * last 40 bytes, but for the sake of simplicity, we just hardcode the tick
 * value that fits the original FANFAREA.IMF for now. To arrive at the value
 * given below, I added up all the delays in that IMF file until the 40th
 * byte is reached.
 */
const auto TIME_FOR_VIDEO_PLAYBACK = engine::fastTicksToTime(1661);

/* This is the additional delay mentioned above, which allows for a tiny bit
 * more time of music playback (~ 214 ms).
 */
const auto TOTAL_TIME = TIME_FOR_VIDEO_PLAYBACK + engine::fastTicksToTime(60);

}


ApogeeLogo::ApogeeLogo(GameMode::Context context)
  : mMoviePlayer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mLogoMovie(context.mpResources->loadMovie("NUKEM2.F5"))
{
}


void ApogeeLogo::start() {
  mpServiceProvider->playMusic("FANFAREA.IMF");
  mMoviePlayer.playMovie(mLogoMovie, 35);
  mElapsedTime = 0.0;
}


void ApogeeLogo::updateAndRender(engine::TimeDelta timeDelta) {
  mElapsedTime += timeDelta;

  if (mElapsedTime < TIME_FOR_VIDEO_PLAYBACK) {
    mMoviePlayer.updateAndRender(timeDelta);
  }
}


bool ApogeeLogo::isFinished() const {
  return mElapsedTime >= TOTAL_TIME;
}

}}
