/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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


namespace rigel::data {

constexpr auto ENABLE_VSYNC_DEFAULT = true;
constexpr auto MUSIC_VOLUME_DEFAULT = 1.0f;
constexpr auto SOUND_VOLUME_DEFAULT = 1.0f;

struct GameOptions {
  // Graphics
  bool mEnableVsync = ENABLE_VSYNC_DEFAULT;

  // Sound
  float mMusicVolume = MUSIC_VOLUME_DEFAULT;
  float mSoundVolume = SOUND_VOLUME_DEFAULT;
  bool mMusicOn = true;
  bool mSoundOn = true;

  // Enhancements
  bool mWidescreenModeOn = false;
};

}
