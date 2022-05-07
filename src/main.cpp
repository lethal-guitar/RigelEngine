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
#include "frontend/user_profile.hpp"

#include "game_main.hpp"

#include <filesystem>

#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <variant>

#include <SDL_main.h>
#include <SDL_messagebox.h>

RIGEL_DISABLE_WARNINGS
#include <loguru.hpp>
#include <lyra/lyra.hpp>
RIGEL_RESTORE_WARNINGS


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


static void enableDpiAwareness()
{
  SetProcessDPIAware();
}

#else

static std::optional<rigel::base::ScopeGuard> win32ReenableStdIo()
{
  return std::nullopt;
}


static void enableDpiAwareness() { }

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

  auto showHelp = false;
  CommandLineOptions config;

  // clang-format off
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

          const auto positionParts = strings::split(positionSpec, ',');
          config.mPlayerPosition = base::Vec2{
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
  // clang-format on

  const auto parseResult = optionsParser.parse({argc, argv});

  if (showHelp)
  {
    std::cout << optionsParser << '\n';
    return 0;
  }

  if (!parseResult)
  {
    std::cerr << "ERROR: " << parseResult.message() << "\n\n";
    std::cerr << optionsParser << '\n';
    return -1;
  }

  if (!config.mGamePath.empty() && config.mGamePath.back() != '/')
  {
    config.mGamePath += "/";
  }

  return config;
}


void initializeLogging(int argc, char** argv)
{
  loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;
  loguru::init(argc, argv);

  if (const auto oPreferencesPath = createOrGetPreferencesPath())
  {
    const auto logFilePath = *oPreferencesPath / "Log.txt";
    loguru::add_file(
      logFilePath.u8string().c_str(), loguru::Append, loguru::Verbosity_MAX);
  }
}


int runGame(const CommandLineOptions& config)
{
  enableDpiAwareness();

  try
  {
    return gameMain(config);
  }
  catch (const std::exception& ex)
  {
    LOG_F(ERROR, "%s", ex.what());
    showErrorBox(ex.what());
    return -2;
  }
  catch (...)
  {
    LOG_F(ERROR, "Unknown error");
    showErrorBox("Unknown error");
    return -3;
  }
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

  const auto configOrExitCode = parseArgs(argc, argv);

  return base::match(
    configOrExitCode,
    [&](const CommandLineOptions& config) {
      // Once we're ready to run, detach from the console. See comment above
      // for why we're doing this.
      win32IoGuard.reset();

      initializeLogging(argc, argv);

      return runGame(config);
    },
    [](const int exitCode) { return exitCode; });
}
