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

#include "high_score_list.hpp"

#include "ui/duke_script_runner.hpp"

#include "game_service_provider.hpp"
#include "user_profile.hpp"


namespace rigel::ui {

void setupHighScoreListDisplay(GameMode::Context& context, const int episode) {
  using namespace std::literals;

  auto drawScoreEntry = [&](int yPos, const data::HighScoreEntry& entry) {
    context.mpUiRenderer->drawText(10, yPos, std::to_string(entry.mScore));
    context.mpUiRenderer->drawText(20, yPos, entry.mName);
  };


  runScript(context, "Volume"s + std::to_string(episode + 1));
  while (!context.mpScriptRunner->hasFinishedExecution()) {
    context.mpScriptRunner->updateAndRender(0.0);
  }

  {
    const auto& list = context.mpUserProfile->mHighScoreLists[episode];
    drawScoreEntry(6, list[0]);

    for (auto i = 1u; i < list.size(); ++i) {
      drawScoreEntry(7 + i, list[i]);
    }

    context.mpServiceProvider->fadeInScreen();
  }

  {
    auto awaitInput = data::script::Script{
      data::script::WaitForUserInput{}
    };

    context.mpScriptRunner->executeScript(awaitInput);
  }
}

}
