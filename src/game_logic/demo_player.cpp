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
#include "common/game_service_provider.hpp"
#include "game_logic/game_world.hpp"


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
  , mpWorld(std::make_unique<GameWorld>(
      &mPlayerModel,
      demoSessionId(0),
      context,
      std::nullopt,
      true,
      mFrames[0].mInput))
{
}


void DemoPlayer::updateAndRender(const engine::TimeDelta dt)
{
  if (isFinished())
  {
    return;
  }

  mElapsedTime += dt;

  if (mElapsedTime >= GAME_LOGIC_UPDATE_DELAY)
  {
    mpWorld->updateGameLogic(mFrames[mCurrentFrameIndex].mInput);
    ++mCurrentFrameIndex;

    mElapsedTime -= GAME_LOGIC_UPDATE_DELAY;
  }

  mpWorld->render();
  mpWorld->processEndOfFrameActions();

  if (
    mCurrentFrameIndex < mFrames.size() &&
    mFrames[mCurrentFrameIndex].mNextLevel)
  {
    mContext.mpServiceProvider->fadeOutScreen();

    ++mCurrentFrameIndex;
    ++mLevelIndex;
    mPlayerModel.resetForNewLevel();
    mpWorld = std::make_unique<GameWorld>(
      &mPlayerModel, demoSessionId(mLevelIndex), mContext);

    mContext.mpServiceProvider->fadeInScreen();
  }
}


bool DemoPlayer::isFinished() const
{
  return mCurrentFrameIndex >= mFrames.size();
}

} // namespace rigel::game_logic
