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

#include "base/string_utils.hpp"
#include "base/warnings.hpp"

#include "game_main.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#if RIGEL_HAS_BOOST

RIGEL_DISABLE_WARNINGS
  #include <boost/program_options.hpp>
RIGEL_RESTORE_WARNINGS

namespace po = boost::program_options;

#endif


using namespace rigel;

namespace
{

void showBanner()
{
  std::cout
    << "================================================================================\n"
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

#if RIGEL_HAS_BOOST

auto parseLevelToJumpTo(const std::string& levelToPlay)
{
  if (levelToPlay.size() != 2)
  {
    throw std::invalid_argument("Invalid level name");
  }

  const auto episode = static_cast<int>(levelToPlay[0] - 'L');
  const auto level = static_cast<int>(levelToPlay[1] - '0') - 1;

  if (episode < 0 || episode >= 4 || level < 0 || level >= 8)
  {
    throw std::invalid_argument(
      std::string("Invalid level name: ") + levelToPlay);
  }
  return std::make_pair(episode, level);
}


auto parseDifficulty(const std::string& difficultySpec)
{
  if (difficultySpec == "easy")
  {
    return data::Difficulty::Easy;
  }
  else if (difficultySpec == "medium")
  {
    return data::Difficulty::Medium;
  }
  else if (difficultySpec == "hard")
  {
    return data::Difficulty::Hard;
  }

  throw std::invalid_argument(
    std::string("Invalid difficulty: ") + difficultySpec);
}


base::Vector parsePlayerPosition(const std::string& playerPosString)
{
  const std::vector<std::string> positionParts =
    strings::split(playerPosString, ',');

  if (
    positionParts.size() != 2 || positionParts[0].empty() ||
    positionParts[1].empty())
  {
    throw std::invalid_argument(
      "Invalid x/y-position (specify using '<X>,<Y>')");
  }

  return base::Vector{std::stoi(positionParts[0]), std::stoi(positionParts[1])};
}

#endif

} // namespace


int main(int argc, char** argv)
{
  showBanner();

#if RIGEL_HAS_BOOST
  CommandLineOptions config;

  // clang-format off
  po::options_description optionsDescription("Options");
  optionsDescription.add_options()
    ("help,h", "Show command line help message")
    ("skip-intro,s",
     po::bool_switch(&config.mSkipIntro),
     "Skip intro movies/Apogee logo, go straight to main menu")
    ("play-level,l",
     po::value<std::string>(),
     "Directly jump to given map, skipping intro/menu etc.")
    ("difficulty",
     po::value<std::string>(),
     "Difficulty to use when jumping to a level via 'play-level'")
    ("player-pos",
     po::value<std::string>(),
     "Specify position to place the player at (to be used in conjunction with\n"
     "'play-level')")
    ("debug-mode,d",
     po::bool_switch(&config.mDebugModeEnabled),
     "Enable debugging features")
    ("play-demo",
     po::bool_switch(&config.mPlayDemo),
     "Play pre-recorded demo")
    ("game-path",
     po::value<std::string>(&config.mGamePath)->default_value(""),
     "Path to original game's installation. Can also be given as positional "
     "argument. If not provided here, a folder browser ui will ask for it");
  // clang-format on

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

    if (options.count("help"))
    {
      std::cout << optionsDescription << '\n';
      return 0;
    }

    if (options.count("play-level"))
    {
      auto sessionId = data::GameSessionId{};
      std::tie(sessionId.mEpisode, sessionId.mLevel) =
        parseLevelToJumpTo(options["play-level"].as<std::string>());

      config.mLevelToJumpTo = sessionId;
    }

    if (options.count("difficulty"))
    {
      if (!options.count("play-level"))
      {
        throw std::invalid_argument(
          "This option requires also using the play-level option");
      }

      config.mLevelToJumpTo->mDifficulty =
        parseDifficulty(options["difficulty"].as<std::string>());
    }

    if (options.count("player-pos"))
    {
      if (!options.count("play-level"))
      {
        throw std::invalid_argument(
          "This option requires also using the play-level option");
      }

      config.mPlayerPosition =
        parsePlayerPosition(options["player-pos"].as<std::string>());
    }

    if (!config.mGamePath.empty() && config.mGamePath.back() != '/')
    {
      config.mGamePath += "/";
    }

    gameMain(config);
  }
  catch (const po::error& err)
  {
    std::cerr << "ERROR: " << err.what() << "\n\n";
    std::cerr << optionsDescription << '\n';
    return -1;
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
#else
  try
  {
    CommandLineOptions config;

    if (argc > 1)
    {
      config.mGamePath = argv[1];

      if (!config.mGamePath.empty() && config.mGamePath.back() != '/')
      {
        config.mGamePath += "/";
      }
    }

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
#endif

  return 0;
}
