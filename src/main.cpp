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

#include "base/warnings.hpp"

#include "game_main.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
RIGEL_RESTORE_WARNINGS

#include <iostream>
#include <stdexcept>
#include <string>


using namespace rigel;
using namespace std;

namespace ba = boost::algorithm;
namespace po = boost::program_options;


namespace {

void showBanner() {
  cout <<
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

  StartupOptions config;

  po::options_description optionsDescription("Options");
  optionsDescription.add_options()
    ("help,h", "Show command line help message")
    ("skip-intro,s",
     po::bool_switch(&config.mSkipIntro),
     "Skip intro movies/Apogee logo, go straight to main menu")
    ("play-level,l",
     po::value<string>(),
     "Directly jump to given map, skipping intro/menu etc.")
    ("player-pos",
     po::value<string>(),
     "Specify position to place the player at (to be used in conjunction with\n"
     "'play-level')")
    ("game-path",
     po::value<string>(&config.mGamePath),
     "Path to original game's installation. Can also be given as positional "
     "argument.");

  po::positional_options_description positionalArgsDescription;
  positionalArgsDescription.add("game-path", -1);

  try
  {
    po::variables_map options;
    po::store(
      po::command_line_parser(argc, argv)
        .options(optionsDescription)
        .positional(positionalArgsDescription)
        .run(),
      options);
    po::notify(options);

    if (options.count("help")) {
      cout << optionsDescription << '\n';
      return 0;
    }

    if (options.count("play-level")) {
      const auto levelToPlay = options["play-level"].as<string>();
      if (levelToPlay.size() != 2) {
        throw invalid_argument("Invalid level name");
      }

      const auto episode = static_cast<int>(levelToPlay[0] - 'L');
      const auto level = static_cast<int>(levelToPlay[1] - '0') - 1;

      if (episode < 0 || episode >= 4 || level < 0 || level >= 8) {
        throw invalid_argument(string("Invalid level name: ") + levelToPlay);
      }

      config.mLevelToJumpTo = std::make_pair(episode, level);
    }

    if (options.count("player-pos")) {
      if (!options.count("play-level")) {
        throw invalid_argument(
          "This option requires also using the play-level option");
      }

      const auto playerPosString = options["player-pos"].as<string>();
      std::vector<std::string> positionParts;
      ba::split(positionParts, playerPosString, ba::is_any_of(","));

      if (
        positionParts.size() != 2 ||
        positionParts[0].empty() ||
        positionParts[1].empty()
      ) {
        throw invalid_argument(
          "Invalid x/y-position (specify using '<X>,<Y>')");
      }

      const auto position = base::Vector{
        std::stoi(positionParts[0]),
        std::stoi(positionParts[1])
      };
      config.mPlayerPosition = position;
    }

    if (!config.mGamePath.empty() && config.mGamePath.back() != '/') {
      config.mGamePath += "/";
    }

    gameMain(config);
  }
  catch (const po::error& err)
  {
    cerr << "ERROR: " << err.what() << "\n\n";
    cerr << optionsDescription << '\n';
    return -1;
  }
  catch (const std::exception& ex)
  {
    cerr << "ERROR: " << ex.what() << '\n';
    return -2;
  }
  catch (...)
  {
    cerr << "UNKNOWN ERROR\n";
    return -3;
  }

  return 0;
}
