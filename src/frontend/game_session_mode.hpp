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

#include "common/game_mode.hpp"
#include "data/player_model.hpp"
#include "ui/bonus_screen.hpp"
#include "ui/episode_end_sequence.hpp"

#include "game_runner.hpp"

#include <variant>

namespace rigel::data { struct SavedGame; }


namespace rigel {

class GameSessionMode : public GameMode {
public:
  GameSessionMode(
    const data::GameSessionId& sessionId,
    Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt);

  GameSessionMode(const data::SavedGame& save, Context context);

  std::unique_ptr<GameMode> updateAndRender(
    engine::TimeDelta dt,
    const std::vector<SDL_Event>& events) override;

private:
  void handleEvent(const SDL_Event& event);
  template<typename StageT>
  void fadeToNewStage(StageT& stage);
  void finishGameSession();
  void enterHighScore(std::string_view name);

private:
  struct HighScoreNameEntry {
    ui::TextEntryWidget mNameEntryWidget;
  };

  struct HighScoreListDisplay {};

  using SessionStage = std::variant<
    std::unique_ptr<GameRunner>,
    ui::BonusScreen,
    ui::EpisodeEndSequence,
    HighScoreNameEntry,
    HighScoreListDisplay
  >;

  data::PlayerModel mPlayerModel;
  SessionStage mCurrentStage;
  const int mEpisode;
  int mCurrentLevelNr;
  const data::Difficulty mDifficulty;
  Context mContext;
};

}
