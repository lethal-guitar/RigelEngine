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

// Welcome to the RigelEngine code base! If you are looking for the place in
// the code where everything starts, you found it. This file contains the
// executable's main() function entry point. Its responsibility is parsing
// command line options, and then handing off control to gameMain(). Most of
// the interesting stuff like the main loop, initialization, and management of
// game modes happens in there, so if you're looking for any of these things,
// you might want to hop over to game_main.cpp instead of looking at this file
// here.

#include "base/warnings.hpp"

#include "game_main.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <lyra/lyra.hpp>
RIGEL_RESTORE_WARNINGS

#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>


using namespace rigel;

namespace ba = boost::algorithm;


namespace {

void showBanner() {
  std::cout <<
    "================================================================================\n"
    "                            Welcome to RIGEL ENGINE!\n"
    "\n"
    "  A modern reimplementation of the game Duke Nukem II, originally released in\n"
    "  1993 for MS-DOS by Apogee Software.\n"
    "\n"
    "You need the original game's data files in order to play, e.g. the freely\n"
    "available shareware version.\n"
    "\n"
    "Rigel Engine Copyright (C) 2016, Nikolai Wuttke.\n"
    "Rigel Engine comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n"
    "are welcome to redistribute it under certain conditions.\n"
    "For details, see https://www.gnu.org/licenses/gpl-2.0.html\n"
    "================================================================================\n"
    "\n";
}

}


int main(int argc, char** argv) {
  showBanner();

  auto showHelp = false;
  CommandLineOptions config;

  auto optionsParser = lyra::help(showHelp)
    | lyra::opt(config.mSkipIntro)["-s"]["--skip-intro"]
      .help("Skip intro movies/Apogee logo, go straight to main menu")
    | lyra::opt(config.mDebugModeEnabled)["-d"]["--debug-mode"]
      .help("Enable debugging features")
    | lyra::opt(config.mPlayDemo)["--play-demo"]
      .help("Play pre-recorded demo")
    | lyra::group([&](const lyra::group&){})
      .add_argument(lyra::opt([&](const std::string& levelSpec){
          config.mLevelToJumpTo = data::GameSessionId{
            static_cast<int>(levelSpec[0] - 'L'),
            static_cast<int>(levelSpec[1] - '0') - 1};
        }, "level name")
        ["-l"]["--play-level"]
        .help("Directly jump to given map, skipping intro/menu etc.")
        .required()
        .choices([](const std::string& levelSpec) {
          std::regex levelRegex{"(L|M|N|O)[1-8]"};
          return std::regex_match(levelSpec, levelRegex);
        }))
      .add_argument(lyra::opt([&](const std::string& difficultySpec) {
          if (!config.mLevelToJumpTo) { return; }

          if (difficultySpec == "easy") {
            config.mLevelToJumpTo->mDifficulty = data::Difficulty::Easy;
          } else if (difficultySpec == "medium") {
            config.mLevelToJumpTo->mDifficulty = data::Difficulty::Medium;
          } else if (difficultySpec == "hard") {
            config.mLevelToJumpTo->mDifficulty = data::Difficulty::Hard;
          }
        }, "easy|medium|hard")
        ["--difficulty"]
        .help("Difficulty to use when jumping to a level")
        .choices("easy", "medium", "hard"))
      .add_argument(lyra::opt([&](const std::string& positionSpec) {
          if (!config.mLevelToJumpTo) { return; }

          std::vector<std::string> positionParts;
          ba::split(positionParts, positionSpec, ba::is_any_of(","));
          config.mPlayerPosition = base::Vector{
            std::stoi(positionParts[0]), std::stoi(positionParts[1])};
        }, "x,y")
        ["--player-pos"]
        .help("Position to place the player at when jumping to a level")
        .choices([](const std::string& positionSpec) {
          std::regex positionRegex{"([0-9]+),([0-9]+)"};
          return std::regex_match(positionSpec, positionRegex);
        }))
    | lyra::arg(config.mGamePath, "game path")
      .help(
        "Path to original game's installation. If not provided here, "
        "the game will show a folder browser UI.")
  ;

  const auto parseResult = optionsParser.parse({argc, argv});

  if (showHelp) {
    std::cout << optionsParser << '\n';
    return 0;
  }

  if (!parseResult) {
    std::cerr << "ERROR: " << parseResult.errorMessage() << "\n\n";
    std::cerr << optionsParser << '\n';
    return -1;
  }

  if (!config.mGamePath.empty() && config.mGamePath.back() != '/') {
    config.mGamePath += "/";
  }

  try {
    gameMain(config);
  }
  catch (const std::exception& ex)
  {
    std::cerr << "ERROR: " << ex.what() << '\n';
    return -2;
  }
  catch (...)
  {
    std::cerr << "UNKNOWN ERROR\n";
    return -3;
  }

  return 0;
}
