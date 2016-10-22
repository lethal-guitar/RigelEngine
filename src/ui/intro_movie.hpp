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

#include <data/movie.hpp>
#include <ui/movie_player.hpp>

#include <game_mode.hpp>

#include <cstddef>
#include <vector>


namespace rigel { namespace ui {

class IntroMovie {
public:
  explicit IntroMovie(GameMode::Context context);

  void start();

  void updateAndRender(engine::TimeDelta dt);

  bool isFinished() const;

private:
  void startNextMovie();

  struct PlaybackConfig {
    data::Movie mMovie;

    const int mFrameDelay;
    const int mRepetitions;
    const ui::MoviePlayer::FrameCallbackFunc mFrameCallback;
  };

  using PlaybackConfigList = std::vector<PlaybackConfig>;

  PlaybackConfigList createConfigurations(
    const loader::ResourceBundle& resources);

private:
  IGameServiceProvider* mpServiceProvider;
  ui::MoviePlayer mMoviePlayer;

  PlaybackConfigList mMovieConfigurations;
  std::size_t mCurrentConfiguration;
};


}}
