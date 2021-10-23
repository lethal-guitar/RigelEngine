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

#include "base/defer.hpp"
#include "base/match.hpp"
#include "base/string_utils.hpp"
#include "base/warnings.hpp"

#include "game_main.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

#include <SDL_main.h>
#include <SDL_messagebox.h>

#if RIGEL_HAS_BOOST

RIGEL_DISABLE_WARNINGS
  #include <boost/program_options.hpp>
RIGEL_RESTORE_WARNINGS

namespace po = boost::program_options;

#endif

#ifdef _WIN32

  #include <stdio.h>
  #include <windows.h>

static std::optional<rigel::base::ScopeGuard> win32ReenableStdIo()
{
  if (AttachConsole(ATTACH_PARENT_PROCESS))
  {
    std::cin.clear();
    std::cout.flush();
    std::cerr.flush();

    FILE* fp = nullptr;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    std::cout << std::endl;

    return rigel::base::defer([]() {
      std::cout.flush();
      std::cerr.flush();

      // This is a hack to make the console output behave like it does when
      // running a genuine console app (i.e. subsystem set to console).
      // The thing is that even though we attach to the console that has
      // launched us, the console itself is not actually waiting for our
      // process to terminate, since it treats us as a GUI application.
      // This means that we can write our stdout/stderr to the console, but
      // the console won't show a new prompt after our process has terminated
      // like it would do with a console application. This makes command line
      // usage awkward because users need to press enter once after each
      // invocation of RigelEngine in order to get a new prompt.
      // By sending a enter key press message to the parent console, we do this
      // automatically.
      SendMessageA(GetConsoleWindow(), WM_CHAR, VK_RETURN, 0);
      FreeConsole();
    });
  }

  return std::nullopt;
}

#else

static std::optional<rigel::base::ScopeGuard> win32ReenableStdIo()
{
  return std::nullopt;
}

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


void showErrorBox(const char* message)
{
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, nullptr);
}


bool isExpectedExeName(const char* exeName)
{
  namespace fs = std::filesystem;

  const auto executablePath = fs::u8path(exeName);
  return strings::startsWith(executablePath.stem().u8string(), "RigelEngine");
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


base::Vector parsePlayerPosition(std::string_view playerPosString)
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


std::variant<CommandLineOptions, int> parseArgs(int argc, char** argv)
{
  if (!isExpectedExeName(argv[0]))
  {
    // If the executable has been renamed, ignore any command line arguments.
    // This is to facilitate using RigelEngine as an executable replacement
    // for the Steam version of Duke2, which uses DosBox normally and passes
    // various arguments that RigelEngine doesn't know about.
    std::cerr
      << "Executable has been renamed, ignoring all command line arguments!\n";

    return CommandLineOptions{};
  }

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

    return config;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "ERROR: " << ex.what() << "\n\n";
    std::cerr << optionsDescription << '\n';
    return -1;
  }
#else
  CommandLineOptions config;

  if (argc > 1)
  {
    config.mGamePath = argv[1];

    if (!config.mGamePath.empty() && config.mGamePath.back() != '/')
    {
      config.mGamePath += "/";
    }
  }

  return config;
#endif
}

} // namespace


int main(int argc, char** argv)
{
  // On Windows, RigelEngine is a GUI application (subsystem win32), which
  // means that it can't be used as a command-line application - stdout and
  // stdin are not connected to the terminal that launches the executable in
  // case of a GUI application.
  // However, it's possible to detect that we've been launched from a terminal,
  // and then manually attach our stdin/stdout to that terminal. This makes
  // our command line interface usable on Windows.
  // It's not perfect, because the terminal itself doesn't actually know
  // that a process it has launched has now attached to it, so it keeps happily
  // accepting user input, it doesn't wait for our process to terminate like
  // it normally does when running a console application. But since we don't
  // need interactive command line use, it's good enough for our case - we
  // can output some text to the terminal and then detach again.
  auto win32IoGuard = win32ReenableStdIo();

  showBanner();

  try
  {
    const auto configOrExitCode = parseArgs(argc, argv);

    return base::match(
      configOrExitCode,
      [&](const CommandLineOptions& config) {
        // Once we're ready to run, detach from the console. See comment above
        // for why we're doing this.
        win32IoGuard.reset();

        return gameMain(config);
      },

      [](const int exitCode) { return exitCode; });
  }
  catch (const std::exception& ex)
  {
    showErrorBox(ex.what());
    std::cerr << "ERROR: " << ex.what() << '\n';
    return -2;
  }
  catch (...)
  {
    showErrorBox("Unknown error");
    std::cerr << "UNKNOWN ERROR\n";
    return -3;
  }
}
