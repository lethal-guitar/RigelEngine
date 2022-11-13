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

#include "intro_demo_loop_mode.hpp"

#include "menu_mode.hpp"

#include "assets/resource_loader.hpp"
#include "base/match.hpp"
#include "frontend/game_service_provider.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/menu_navigation.hpp"

#include <stdexcept>


namespace rigel
{

IntroDemoLoopMode::IntroDemoLoopMode(Context context, const Type type)
  : mContext(context)
{
  if (type == Type::Regular)
  {
    try
    {
      mSteps.push_back(ui::IntroMovie(context));
    }
    catch (const std::runtime_error&) // Skip movies if they can't be opened
    {
    }

    mSteps.push_back(Credits{});
    mSteps.push_back(game_logic::DemoPlayer(context));

    try
    {
      mSteps.push_back(ui::ApogeeLogo(context));
    }
    catch (const std::runtime_error&)
    {
    }
  }
  else
  {
    if (type == Type::AtFirstLaunch)
    {
      mSteps.push_back(HypeScreen{});
    }

    try
    {
      mSteps.push_back(ui::ApogeeLogo(context));
    }
    catch (const std::runtime_error&)
    {
    }

    try
    {
      mSteps.push_back(ui::IntroMovie(context));
    }
    catch (const std::runtime_error&)
    {
    }

    mSteps.push_back(Story{});
    mSteps.push_back(Credits{});
    mSteps.push_back(game_logic::DemoPlayer(context));
  }

  startCurrentStep();
}


bool IntroDemoLoopMode::handleEvent(const SDL_Event& event)
{
  if (!ui::isButtonPress(event))
  {
    return false;
  }

  return base::match(
    mSteps[mCurrentStep],
    [this](ui::ApogeeLogo& state) {
      // Pressing any key on the Apogee Logo skips forward to the intro movie
      updateCurrentStep(0.0);
      mContext.mpServiceProvider->fadeOutScreen();
      advanceToNextStep();
      updateCurrentStep(0.0);
      mContext.mpServiceProvider->fadeInScreen();
      return false;
    },

    [&](ui::IntroMovie& state) {
      mContext.mpServiceProvider->stopAllSounds();
      return true;
    },

    [&](Credits& state) { return true; },

    [&](game_logic::DemoPlayer& state) { return true; },

    [&](auto&& state) {
      mContext.mpScriptRunner->handleEvent(event);
      return mContext.mpScriptRunner->hasFinishedExecution() &&
        mContext.mpScriptRunner->result()->mTerminationType ==
        ui::DukeScriptRunner::ScriptTerminationType::AbortedByUser;
    });
}


std::unique_ptr<GameMode> IntroDemoLoopMode::updateAndRender(
  const engine::TimeDelta dt,
  const std::vector<SDL_Event>& events)
{
  for (const auto& event : events)
  {
    const auto shouldQuit = handleEvent(event);
    if (shouldQuit)
    {
      updateCurrentStep(0.0);
      mContext.mpServiceProvider->fadeOutScreen();
      return std::make_unique<MenuMode>(mContext);
    }
  }

  updateCurrentStep(dt);

  if (isCurrentStepFinished())
  {
    advanceToNextStep();
  }

  return {};
}


void IntroDemoLoopMode::startCurrentStep()
{
  using std::begin;
  using std::end;

  base::match(
    mSteps[mCurrentStep],
    [this](Story& state) { runScript(mContext, "&Story"); },

    [this](HypeScreen& state) { runScript(mContext, "HYPE"); },

    [this](Credits& state) {
      auto creditsScript = mContext.mpScripts->at("&Credits");

      // The credits screen is shown twice as long in the registered version.
      // This makes the timing equivalent between the versions, only that the
      // shareware version will switch to the order info screen after half the
      // time has elapsed.
      if (mContext.mpServiceProvider->isSharewareVersion())
      {
        creditsScript.emplace_back(data::script::Delay{700});
        const auto orderInfoScript = mContext.mpScripts->at("Q_ORDER");
        creditsScript.insert(
          end(creditsScript), begin(orderInfoScript), end(orderInfoScript));
        creditsScript.emplace_back(data::script::Delay{700});
      }
      else
      {
        creditsScript.emplace_back(data::script::Delay{700 * 2});
      }

      mContext.mpScriptRunner->executeScript(creditsScript);
    },

    [&](game_logic::DemoPlayer& state) {
      state = game_logic::DemoPlayer{mContext};
      mContext.mpServiceProvider->fadeOutScreen();
      state.updateAndRender(0.0);
      mContext.mpServiceProvider->fadeInScreen();
    },

    [](auto&& state) { state.start(); });
}


void IntroDemoLoopMode::updateCurrentStep(const engine::TimeDelta dt)
{
  std::visit(
    [&](auto&& state) {
      if constexpr (std::
                      is_base_of_v<ScriptedStep, std::decay_t<decltype(state)>>)
      {
        mContext.mpScriptRunner->updateAndRender(dt);
      }
      else
      {
        state.updateAndRender(dt);
      }
    },
    mSteps[mCurrentStep]);
}


bool IntroDemoLoopMode::isCurrentStepFinished() const
{
  return std::visit(
    [&](auto&& state) {
      if constexpr (std::
                      is_base_of_v<ScriptedStep, std::decay_t<decltype(state)>>)
      {
        return mContext.mpScriptRunner->hasFinishedExecution();
      }
      else
      {
        return state.isFinished();
      }
    },
    mSteps[mCurrentStep]);
}


void IntroDemoLoopMode::advanceToNextStep()
{
  const auto isOneTimeStep =
    std::holds_alternative<Story>(mSteps[mCurrentStep]) ||
    std::holds_alternative<HypeScreen>(mSteps[mCurrentStep]);
  if (isOneTimeStep)
  {
    mSteps.erase(next(begin(mSteps), mCurrentStep));
  }
  else
  {
    ++mCurrentStep;

    if (mCurrentStep >= mSteps.size())
    {
      mCurrentStep = 0;
    }
  }

  startCurrentStep();
}

} // namespace rigel
