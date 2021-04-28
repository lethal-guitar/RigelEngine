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

#include <base/warnings.hpp>
#include <common/user_profile.hpp>
#include <loader/file_utils.hpp>

RIGEL_DISABLE_WARNINGS
#include <nlohmann/json.hpp>
RIGEL_RESTORE_WARNINGS

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>


namespace
{

constexpr auto INDENTATION_WIDTH = 2;


void printBanner(const std::filesystem::path& prefsDirPath)
{
  std::cout << "== Rigel Engine user profile tool ==\n"
               "\n"
               "User profile path: \""
            << prefsDirPath.u8string()
            << "\"\n"
               "\n";
}


void printUsage()
{
  std::cout <<
    R"(Usage:
  UserProfileTool <command>

With command being 'encode' or 'decode'. Both commands operate in-place in
the user profile directory.

encode - reads JSON version of profile, and writes binary version
decode - reads binary profile file, and writes a JSON version
)";
}


std::string readJsonFile(const std::filesystem::path& path)
{
  auto inFile = std::ifstream{path.u8string()};
  if (inFile.is_open())
  {
    return std::string{
      std::istreambuf_iterator<char>{inFile}, std::istreambuf_iterator<char>{}};
  }

  return "";
}


void writeJsonFile(const std::string& json, const std::filesystem::path& path)
{
  auto outFile = std::ofstream{path.u8string()};
  outFile << json;
}


bool userConfirmed()
{
  std::cout << "WARNING: This will overwrite your current profile.\n"
            << "Proceed? [Y/n] ";

  char confirmation = 'n';
  std::cin >> confirmation;

  return confirmation == 'y' || confirmation == 'Y';
}

} // namespace


int main(int argc, char** argv)
{
  const auto USER_PROFILE_FILENAME =
    std::string{rigel::USER_PROFILE_BASE_NAME} +
    rigel::USER_PROFILE_FILE_EXTENSION;
  const auto JSON_USER_PROFILE_FILENAME =
    std::string{rigel::USER_PROFILE_BASE_NAME} + ".json";

  using namespace std::literals;

  namespace fs = std::filesystem;

  try
  {
    const auto prefsDirPath = rigel::createOrGetPreferencesPath();
    if (!prefsDirPath)
    {
      std::cerr << "ERROR: Failed to get preferences path\n";
      return 1;
    }

    printBanner(*prefsDirPath);

    if (argc < 2)
    {
      printUsage();
      return 1;
    }


    const auto command = argv[1];

    if (command == "decode"s)
    {
      const auto profileFilePath = *prefsDirPath / USER_PROFILE_FILENAME;
      if (!fs::exists(profileFilePath))
      {
        std::cerr << "ERROR: No profile file found\n";
        return 1;
      }

      const auto buffer = rigel::loader::loadFile(profileFilePath);
      const auto profile = nlohmann::json::from_msgpack(buffer);
      const auto outFilePath = *prefsDirPath / JSON_USER_PROFILE_FILENAME;
      writeJsonFile(profile.dump(INDENTATION_WIDTH), outFilePath);

      std::cout << "Profile successfully decoded. Find the JSON file at:\n"
                   "\t\""
                << outFilePath.u8string() << "\"\n";
    }
    else if (command == "encode"s)
    {
      const auto jsonFilePath = *prefsDirPath / JSON_USER_PROFILE_FILENAME;
      if (!fs::exists(jsonFilePath))
      {
        std::cerr << "ERROR: No decoded profile (JSON file) found\n";
        return 1;
      }

      const auto jsonProfileText = readJsonFile(jsonFilePath);
      const auto jsonProfile = nlohmann::json::parse(jsonProfileText);
      const auto serializedBuffer = nlohmann::json::to_msgpack(jsonProfile);

      if (userConfirmed())
      {
        const auto profileFilePath = *prefsDirPath / USER_PROFILE_FILENAME;
        rigel::loader::saveToFile(serializedBuffer, profileFilePath);

        std::cout << "Profile successfully encoded.\n";
      }
    }
    else
    {
      printUsage();
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << "ERROR: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}
