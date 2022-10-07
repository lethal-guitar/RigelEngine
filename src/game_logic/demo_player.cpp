/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "demo_player.hpp"

#include "assets/resource_loader.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/game_world.hpp"
#include "game_logic_classic/game_world_classic.hpp"


namespace rigel::game_logic
{

namespace
{

constexpr int DEMO_EPISODE = 0;
constexpr int DEMO_LEVELS[] = {0, 2, 4, 6};
constexpr auto DEMO_DIFFICULTY = data::Difficulty::Hard;
constexpr std::uint8_t END_OF_DEMO_MARKER = 0xFF;


PlayerInput
  parseInput(const std::uint8_t byte, const PlayerInput& previousInput)
{
  PlayerInput result;

  result.mUp = (byte & 0b1) != 0;
  result.mDown = (byte & 0b10) != 0;
  result.mLeft = (byte & 0b100) != 0;
  result.mRight = (byte & 0b1000) != 0;
  result.mJump.mIsPressed = (byte & 0b10000) != 0;
  result.mFire.mIsPressed = (byte & 0b100000) != 0;
  result.mInteract.mIsPressed = (byte & 0b1) != 0;

  result.mJump.mWasTriggered =
    result.mJump.mIsPressed && !previousInput.mJump.mIsPressed;
  result.mFire.mWasTriggered =
    result.mFire.mIsPressed && !previousInput.mFire.mIsPressed;
  result.mInteract.mWasTriggered =
    result.mInteract.mIsPressed && !previousInput.mInteract.mIsPressed;

  return result;
}


std::vector<DemoInput> loadDemo(const assets::ResourceLoader& resources)
{
  PlayerInput previousInput;
  std::vector<DemoInput> result;

  const auto demoData = resources.file("NUKEM2.MNI");
  for (const auto byte : demoData)
  {
    if (byte == END_OF_DEMO_MARKER)
    {
      break;
    }

    const auto currentInput = parseInput(byte, previousInput);
    const auto switchToNextLevel = (byte & 0b10000000) != 0;
    result.push_back({currentInput, switchToNextLevel});

    previousInput = currentInput;
  }

  return result;
}


data::GameSessionId demoSessionId(const std::size_t levelIndex)
{
  return {DEMO_EPISODE, DEMO_LEVELS[levelIndex], DEMO_DIFFICULTY};
}

} // namespace


DemoPlayer::DemoPlayer(GameMode::Context context)
  : mContext(context)
  , mFrames(loadDemo(*context.mpResources))
{
}


DemoPlayer::~DemoPlayer() = default;
DemoPlayer::DemoPlayer(DemoPlayer&&) = default;
DemoPlayer& DemoPlayer::operator=(DemoPlayer&&) = default;


void DemoPlayer::updateAndRender(const engine::TimeDelta dt)
{
  if (isFinished())
  {
    return;
  }

  if (!mpWorld)
  {
    mpWorld = std::make_unique<GameWorld_Classic>(
      &mPlayerModel,
      demoSessionId(0),
      mContext,
      std::nullopt,
      true,
      mFrames[0].mInput);
  }

  auto changeLevel = false;

  mElapsedTime += dt;

  if (mElapsedTime >= GAME_LOGIC_UPDATE_DELAY)
  {
    mpWorld->updateGameLogic(mFrames[mCurrentFrameIndex].mInput);
    changeLevel = mFrames[mCurrentFrameIndex].mNextLevel;
    ++mCurrentFrameIndex;

    mElapsedTime -= GAME_LOGIC_UPDATE_DELAY;
  }

  mpWorld->render();
  mpWorld->processEndOfFrameActions();

  if (changeLevel && mCurrentFrameIndex < mFrames.size())
  {
    mContext.mpServiceProvider->fadeOutScreen();

    ++mLevelIndex;

    mPlayerModel.resetForNewLevel();

    mpWorld = std::make_unique<GameWorld_Classic>(
      &mPlayerModel,
      demoSessionId(mLevelIndex),
      mContext,
      std::nullopt,
      false,
      mFrames[mCurrentFrameIndex].mInput);
    mpWorld->render();

    mCurrentFrameIndex++;

    mContext.mpServiceProvider->fadeInScreen();
  }
}


bool DemoPlayer::isFinished() const
{
  return mCurrentFrameIndex >= mFrames.size();
}

} // namespace rigel::game_logic
