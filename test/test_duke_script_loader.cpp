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

#include <base/warnings.hpp>
#include <loader/duke_script_loader.hpp>
#include <loader/resource_loader.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS

#include <typeinfo>


using namespace rigel::loader;
using namespace rigel::data::script;

using namespace std;

namespace {

Script loadSingleScript(const string& source) {
  auto sourceWithName = string("TestTestTest\r\n\r\n");
  sourceWithName += source;

  auto scriptBundle = loadScripts(sourceWithName);
  return scriptBundle["TestTestTest"];
}


template<typename ActionT>
auto asType(const Action& action) {
  return std::get<ActionT>(action);
}


template<typename ActionT>
bool isType(const Action& action) {
  return std::holds_alternative<ActionT>(action);
}

}


TEST_CASE("Duke Script files are parsed correctly for base cases") {
  const auto testData =
    "\r\n"
    "Test_Script\r\n"
    "\r\n"
    "//FADEIN\r\n"
    "//END\r\n"
    "\r\n"
    "Another_Test_Script\r\n"
    "\r\n"
    "//FADEOUT\r\n"
    "//DELAY 600\r\n"
    "//LOADRAW MESSAGE.MNI\r\n"
    "//END\r\n";

  const auto scriptBundle = loadScripts(testData);

  SECTION("Script names are parsed correctly") {
    REQUIRE(scriptBundle.size() == 2);
    CHECK(scriptBundle.count("Test_Script") == 1);
    CHECK(scriptBundle.count("Another_Test_Script") == 1);

    const auto& firstScript = scriptBundle.find("Test_Script")->second;
    const auto& secondScript = scriptBundle.find("Another_Test_Script")->second;

    SECTION("Right number of actions is created, end marker is skipped") {
      REQUIRE(firstScript.size() == 1);
      REQUIRE(secondScript.size() == 3);

      SECTION("Correct action types are created") {
        CHECK(isType<FadeIn>(firstScript[0]));

        CHECK(isType<FadeOut>(secondScript[0]));
        REQUIRE(isType<Delay>(secondScript[1]));
        REQUIRE(isType<ShowFullScreenImage>(secondScript[2]));

        SECTION("Parameterized actions contain correct parameters") {
          const auto delay = asType<Delay>(secondScript[1]);
          const auto showImage = asType<ShowFullScreenImage>(secondScript[2]);

          CHECK(delay.amount == 600);
          CHECK(showImage.image == "MESSAGE.MNI");
        }
      }
    }
  }
}

TEST_CASE("White space between commands is ignored") {
  const auto testData =
    "WhiteSpaceTest\r\n"
    "\r\n"
    "\r\n"
    "//FADEIN\r\n"
    "\r\n"
    "\r\n"
    "//FADEOUT\r\n"
    "\r\n"
    "//END\r\n"
    "\r\n"
    "\r\n";

  const auto scriptBundle = loadScripts(testData);
  const auto& testScript = scriptBundle.find("WhiteSpaceTest")->second;

  CHECK(scriptBundle.size() == 1);
  CHECK(testScript.size() == 2);
}

TEST_CASE("Draw Text and Draw Sprite commands are parsed correctly") {

RIGEL_DISABLE_WARNINGS
  // The original Duke Script files sometimes embed specific bytes outside of
  // the ASCII range in the normal source code to indicate certain things.
  // Modern compilers won't accept that without warnings unfortunately..
  const auto testData =
    "//XYTEXT 2 4 Hello World what's up!\r\n"
    "//XYTEXT 22 10 \xEF""14504\r\n"
    "//END\r\n";
RIGEL_RESTORE_WARNINGS

  const auto testScript = loadSingleScript(testData);

  SECTION("Command types are correct") {
    const auto drawText = asType<DrawText>(testScript[0]);
    const auto drawSprite = asType<DrawSprite>(testScript[1]);

    SECTION("Draw Text is parsed correctly") {
      CHECK(drawText.x == 2);
      CHECK(drawText.y == 4);
      CHECK(drawText.text == "Hello World what's up!");
    }

    SECTION("Draw Sprite is parsed correctly, x/y are adjusted") {
      CHECK(drawSprite.x == 24);
      CHECK(drawSprite.y == 11);
      CHECK(drawSprite.spriteId == 145);
      CHECK(drawSprite.frameNumber == 4);
    }
  }
}


TEST_CASE("Draw big text is parsed correctly") {

RIGEL_DISABLE_WARNINGS
  const auto testData =
    "//XYTEXT 2 4 \xF2""Colored text!\r\n"
    "//XYTEXT 2 8 test\xF7""Colored text with leading regular text\r\n"
    "//END\r\n";
RIGEL_RESTORE_WARNINGS

  const auto testScript = loadSingleScript(testData);
  REQUIRE(testScript.size() == 3);

  REQUIRE(isType<DrawBigText>(testScript[0]));
  REQUIRE(isType<DrawText>(testScript[1]));
  REQUIRE(isType<DrawBigText>(testScript[2]));

  SECTION("BigText has correct data") {
    const auto bigText = asType<DrawBigText>(testScript[0]);

    CHECK(bigText.x == 2);
    CHECK(bigText.y == 4);
    CHECK(bigText.colorIndex == 2);
    CHECK(bigText.text == "Colored text!");
  }

  SECTION("Mixed regular and big text results in two text commands") {
    const auto leadingRegularText = asType<DrawText>(testScript[1]);
    CHECK(leadingRegularText.x == 2);
    CHECK(leadingRegularText.y == 8);
    CHECK(leadingRegularText.text == "test");

    const auto bigText = asType<DrawBigText>(testScript[2]);
    CHECK(bigText.x == 2 + 4); // four leading characters in 'test'
    CHECK(bigText.y == 8);
    CHECK(bigText.colorIndex == 7);
    CHECK(bigText.text == "Colored text with leading regular text");
  }
}


TEST_CASE("Get palette command is parsed correctly") {
  const auto testData =
    "PaletteTest\r\n"
    "\r\n"
    "//GETPAL Test.pal\r\n"
    "//END\r\n";

  const auto scriptBundle = loadScripts(testData);
  const auto& testScript = scriptBundle.find("PaletteTest")->second;

  REQUIRE(testScript.size() == 1);

  const auto palette = asType<SetPalette>(testScript[0]);
  CHECK(palette.paletteFile == "Test.pal");
}

TEST_CASE("Message box definition is parsed correctly") {
  const auto testData =
    "//CENTERWINDOW 5 6 24\r\n"
    "//SKLINE\r\n"
    "//CWTEXT This is a\r\n"
    "//CWTEXT test!\r\n"
    "//SKLINE\r\n"
    "//CWTEXT   Hello Leading Space\r\n"
    "//END\r\n";
  const auto testScript = loadSingleScript(testData);

  REQUIRE(testScript.size() == 1);
  const auto msgBox = asType<ShowMessageBox>(testScript[0]);

  CHECK(msgBox.y == 5);
  CHECK(msgBox.height == 6);
  CHECK(msgBox.width == 24);

  vector<string> expectedMessageLines{
    "",
    "This is a",
    "test!",
    "",
    "  Hello Leading Space"
  };
  CHECK(msgBox.messageLines == expectedMessageLines);
}

TEST_CASE("News reporter animation commands are parsed correctly") {
  const auto testData =
    "//BABBLEON 50\r\n"
    "//BABBLEOFF\r\n"
    "//END\r\n";
  const auto script = loadSingleScript(testData);

  REQUIRE(script.size() == 2);

  CHECK(isType<StopNewsReporterAnimation>(script[1]));

  const auto startTalking = asType<AnimateNewsReporter>(script[0]);
  CHECK(startTalking.talkDuration == 50);
}

TEST_CASE("Simple commands are parsed correctly") {
  const auto testData =
    "//WAIT\r\n"
    "//SHIFTWIN\r\n"
    "//EXITTODEMO\r\n"
    "//SHIFTWIN 5\r\n" // the parameter is ignored
    "//KEYS\r\n"
    "//END\r\n";
  const auto script = loadSingleScript(testData);

  REQUIRE(script.size() == 5);

  CHECK(isType<WaitForUserInput>(script[0]));
  CHECK(isType<EnableTextOffset>(script[1]));
  CHECK(isType<EnableTimeOutToDemo>(script[2]));
  CHECK(isType<EnableTextOffset>(script[3]));
  CHECK(isType<ShowKeyBindings>(script[4]));
}

TEST_CASE("ShowSaveSlots is parsed correctly") {
  const auto testData =
    "//GETNAMES 0\r\n"
    "//GETNAMES 5\r\n"
    "//END\r\n";
  const auto script = loadSingleScript(testData);

  REQUIRE(script.size() == 2);

  CHECK(asType<ShowSaveSlots>(script[0]).mSelectedSlot == 0);
  CHECK(asType<ShowSaveSlots>(script[1]).mSelectedSlot == 5);
}

TEST_CASE("Page definitions are parsed correctly") {
  const auto testData =
    "//PAGESSTART\r\n"
    "//FADEOUT\r\n"
    "//WAIT\r\n"
    "\r\n"
    "//APAGE\r\n"
    "//XYTEXT 2 4 Test ABC\r\n"
    "//WAIT\r\n"
    "\r\n"
    "//APAGE\r\n"
    "//DELAY 500\r\n"
    "//BABBLEON 30\r\n"
    "//SHIFTWIN\r\n"
    "//PAGESEND\r\n"
    "//END\r\n";
  const auto script = loadSingleScript(testData);

  REQUIRE(script.size() == 1);
  REQUIRE(isType<std::shared_ptr<PagesDefinition>>(script.front()));

  auto pageDefinition =
    *asType<std::shared_ptr<PagesDefinition>>(script.front());
  REQUIRE(pageDefinition.pages.size() == 3);

  SECTION("Commands after PAGESSTART go into first page") {
    const auto& firstPage = pageDefinition.pages[0];
    REQUIRE(firstPage.size() == 2);
    REQUIRE(isType<FadeOut>(firstPage[0]));
    REQUIRE(isType<WaitForUserInput>(firstPage[1]));

    SECTION("Commands on subsequent pages have correct types") {
      const auto& secondPage = pageDefinition.pages[1];
      REQUIRE(secondPage.size() == 2);
      REQUIRE(isType<DrawText>(secondPage[0]));
      REQUIRE(isType<WaitForUserInput>(secondPage[1]));

      const auto& thirdPage = pageDefinition.pages[2];
      REQUIRE(thirdPage.size() == 3);
      REQUIRE(isType<Delay>(thirdPage[0]));
      REQUIRE(isType<AnimateNewsReporter>(thirdPage[1]));
      REQUIRE(isType<EnableTextOffset>(thirdPage[2]));

      SECTION("Parameters are correct for commands on pages") {
        auto xyText = asType<DrawText>(secondPage[0]);
        CHECK(xyText.x == 2);
        CHECK(xyText.y == 4);
        CHECK(xyText.text == "Test ABC");

        auto delay = asType<Delay>(thirdPage[0]);
        auto babble = asType<AnimateNewsReporter>(thirdPage[1]);
        CHECK(delay.amount == 500);
        CHECK(babble.talkDuration == 30);
      }
    }
  }
}


TEST_CASE("Level hints are parsed correctly") {
  const auto testData =
    "Preceding_Stuff\r\n"
    "\r\n"
    "//FADEOUT\r\n"
    "//WAIT\r\n"
    "//END\r\n"
    "\r\n"
    "Hints\r\n"
    "\r\n"
    "//HELPTEXT 1 3 This is the hint for level 3\r\n"
    "//HELPTEXT 2 2 Hello World\r\n"
    "//END\r\n";

  const auto parsedHints = loadHintMessages(testData);

  CHECK(parsedHints.mHints.size() == 2u);
  CHECK(parsedHints.mHints[0].mMessage == "This is the hint for level 3");
  CHECK(parsedHints.mHints[0].mEpisode == 0);
  CHECK(parsedHints.mHints[0].mLevel == 2);
  CHECK(parsedHints.mHints[1].mMessage == "Hello World");
  CHECK(parsedHints.mHints[1].mEpisode == 1);
  CHECK(parsedHints.mHints[1].mLevel == 1);
}
