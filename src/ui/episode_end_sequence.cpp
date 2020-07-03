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
#include "common/game_service_provider.hpp"
#include "engine/timing.hpp"
#include "ui/menu_navigation.hpp"
#include "ui/utils.hpp"


namespace rigel::ui {

namespace {

constexpr auto EPISODE_END_SCREEN_INITIAL_DELAY = engine::slowTicksToTime(140);

constexpr const char* EPISODE_1_END_IMAGES[] = {
  "END1-3.MNI",
  "END1-1.MNI",
  "END1-2.MNI"
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


std::vector<renderer::OwningTexture> loadImagesForEpisode(
  GameMode::Context context,
  const int episode
) {
  auto createTextures = [&context](const auto& imageFilenames) {
    return utils::transformed(imageFilenames, [&](const char* imageFilename) {
      return ui::fullScreenImageAsTexture(
        context.mpRenderer, *context.mpResources, imageFilename);
    });
  };

  switch (episode) {
    case 0: return createTextures(EPISODE_1_END_IMAGES);
    case 1: return createTextures(EPISODE_2_END_IMAGES);
    case 2: return createTextures(EPISODE_3_END_IMAGES);
    case 3: return createTextures(EPISODE_4_END_IMAGES);
    default: return {};
  }
}

}


EpisodeEndScreen::EpisodeEndScreen(GameMode::Context context, const int episode)
  : mScreenImages(loadImagesForEpisode(context, episode))
  , mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
{
}


void EpisodeEndScreen::updateAndRender(engine::TimeDelta dt) {
  const auto index = std::min(mCurrentImage, mScreenImages.size());
  mScreenImages[index].render(mpRenderer, 0, 0);
}


void EpisodeEndScreen::handleEvent(const SDL_Event& event) {
  if (!ui::isButtonPress(event)) {
    return;
  }

  updateAndRender(0);
  mpServiceProvider->fadeOutScreen();

  if (mCurrentImage < mScreenImages.size()) {
    ++mCurrentImage;

    if (mCurrentImage < mScreenImages.size()) {
      updateAndRender(0);
      mpServiceProvider->fadeInScreen();
    }
  }
}


bool EpisodeEndScreen::finished() const {
  return mCurrentImage >= mScreenImages.size();
}


EpisodeEndSequence::EpisodeEndSequence(
  GameMode::Context context,
  const int episode,
  ui::BonusScreen bonusScreen
)
  : mEndScreen(context, episode)
  , mBonusScreen(std::move(bonusScreen))
  , mpServiceProvider(context.mpServiceProvider)
{
}


void EpisodeEndSequence::updateAndRender(engine::TimeDelta dt) {
  if (mElapsedTime < EPISODE_END_SCREEN_INITIAL_DELAY) {
    mElapsedTime += dt;

    if (mElapsedTime >= EPISODE_END_SCREEN_INITIAL_DELAY) {
      mEndScreen.updateAndRender(0);
      mpServiceProvider->fadeInScreen();
    }

    return;
  }

  if (mEndScreen.finished()) {
    mBonusScreen.updateAndRender(dt);
  } else {
    mEndScreen.updateAndRender(dt);
  }
}


void EpisodeEndSequence::handleEvent(const SDL_Event& event) {
  if (
    mElapsedTime >= EPISODE_END_SCREEN_INITIAL_DELAY &&
    !mEndScreen.finished()
  ) {
    mEndScreen.handleEvent(event);

    if (mEndScreen.finished()) {
      mBonusScreen.updateAndRender(0.0);
      mpServiceProvider->fadeInScreen();
    }
  }
}


bool EpisodeEndSequence::finished() const {
  return mBonusScreen.finished();
}

}
