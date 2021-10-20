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

#include "episode_end_sequence.hpp"

#include "base/container_utils.hpp"
#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "engine/timing.hpp"
#include "ui/menu_navigation.hpp"
#include "ui/utils.hpp"


namespace rigel::ui
{

namespace
{

constexpr auto EPISODE_END_SCREEN_INITIAL_DELAY = engine::slowTicksToTime(140);

constexpr const char* EPISODE_1_END_IMAGES[] = {
  "END1-3.MNI",
  "END1-1.MNI",
  "END1-2.MNI",
};

constexpr const char* EPISODE_2_END_IMAGES[] = {
  "END2-1.MNI",
};

constexpr const char* EPISODE_3_END_IMAGES[] = {
  "END3-1.MNI",
};

constexpr const char* EPISODE_4_END_IMAGES[] = {
  "END4-1.MNI",
  "END4-3.MNI",
};


std::vector<renderer::Texture>
  loadImagesForEpisode(GameMode::Context context, const int episode)
{
  auto createTextures = [&context](const auto& imageFilenames) {
    return utils::transformed(imageFilenames, [&](const char* imageFilename) {
      return ui::fullScreenImageAsTexture(
        context.mpRenderer, *context.mpResources, imageFilename);
    });
  };

  // clang-format off
  switch (episode) {
    case 0: return createTextures(EPISODE_1_END_IMAGES);
    case 1: return createTextures(EPISODE_2_END_IMAGES);
    case 2: return createTextures(EPISODE_3_END_IMAGES);
    case 3: return createTextures(EPISODE_4_END_IMAGES);
    default: return {};
  }
  // clang-format on
}

} // namespace


EpisodeEndScreen::EpisodeEndScreen(GameMode::Context context, const int episode)
  : mScreenImages(loadImagesForEpisode(context, episode))
  , mpServiceProvider(context.mpServiceProvider)
{
}


void EpisodeEndScreen::updateAndRender(engine::TimeDelta dt)
{
  const auto index = std::min(mCurrentImage, mScreenImages.size());
  mScreenImages[index].render(0, 0);
}


void EpisodeEndScreen::handleEvent(const SDL_Event& event)
{
  if (!ui::isButtonPress(event))
  {
    return;
  }

  updateAndRender(0);
  mpServiceProvider->fadeOutScreen();

  if (mCurrentImage < mScreenImages.size())
  {
    ++mCurrentImage;

    if (mCurrentImage < mScreenImages.size())
    {
      updateAndRender(0);
      mpServiceProvider->fadeInScreen();
    }
  }
}


bool EpisodeEndScreen::finished() const
{
  return mCurrentImage >= mScreenImages.size();
}


EpisodeEndSequence::EpisodeEndSequence(
  GameMode::Context context,
  const int episode,
  std::set<data::Bonus> achievedBonuses,
  int scoreWithoutBonuses)
  : mContext(context)
  , mAchievedBonuses(std::move(achievedBonuses))
  , mEpisode(episode)
  , mScoreWithoutBonuses(scoreWithoutBonuses)
{
}


void EpisodeEndSequence::updateAndRender(engine::TimeDelta dt)
{
  base::match(
    mStage,
    [this, dt](InitialWait& state) {
      state.mElapsedTime += dt;

      if (state.mElapsedTime >= EPISODE_END_SCREEN_INITIAL_DELAY)
      {
        startNewStage(ui::EpisodeEndScreen{mContext, mEpisode});
      }
    },

    [this, dt](ui::Duke3DTeaserScreen& duke3DTeaser) {
      duke3DTeaser.updateAndRender(dt);

      if (duke3DTeaser.isFinished())
      {
        startNewStage(
          ui::BonusScreen{mContext, mAchievedBonuses, mScoreWithoutBonuses});
      }
    },

    [dt](auto& state) { state.updateAndRender(dt); });
}


void EpisodeEndSequence::handleEvent(const SDL_Event& event)
{
  base::match(
    mStage,
    [&](ui::EpisodeEndScreen& endScreen) {
      endScreen.handleEvent(event);

      if (endScreen.finished())
      {
        if (mEpisode == 3)
        {
          startNewStage(
            ui::Duke3DTeaserScreen{*mContext.mpResources, mContext.mpRenderer});
        }
        else
        {
          startNewStage(
            ui::BonusScreen{mContext, mAchievedBonuses, mScoreWithoutBonuses});
        }
      }
    },

    [&](ui::Duke3DTeaserScreen&) {
      if (ui::isButtonPress(event))
      {
        startNewStage(
          ui::BonusScreen{mContext, mAchievedBonuses, mScoreWithoutBonuses});
      }
    },

    [&](ui::BonusScreen& screen) { screen.handleEvent(event); },

    [](auto&&) {});
}


bool EpisodeEndSequence::finished() const
{
  return base::match(
    mStage,
    [](const ui::BonusScreen& bonusScreen) { return bonusScreen.finished(); },

    [](auto&&) { return false; });
}


template <typename T>
void EpisodeEndSequence::startNewStage(T&& newStage)
{
  mContext.mpServiceProvider->fadeOutScreen();
  newStage.updateAndRender(0.0);
  mContext.mpServiceProvider->fadeInScreen();

  mStage = std::forward<T>(newStage);
}

} // namespace rigel::ui
