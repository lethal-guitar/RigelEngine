#include "intro_demo_loop_mode.hpp"

#include "menu_mode.hpp"

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "loader/resource_loader.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/menu_navigation.hpp"


namespace rigel {

IntroDemoLoopMode::IntroDemoLoopMode(
  Context context,
  const Type type
)
  : mContext(context)
{
  if (type == Type::Regular) {
    mSteps.push_back(ui::IntroMovie(context));
    mSteps.push_back(Credits{});
    mSteps.push_back(ui::ApogeeLogo(context));
  } else {
    if (type == Type::AtFirstLaunch) {
      mSteps.push_back(HypeScreen{});
    }

    mSteps.push_back(ui::ApogeeLogo(context));
    mSteps.push_back(ui::IntroMovie(context));
    mSteps.push_back(Story{});
    mSteps.push_back(Credits{});
  }

  startCurrentStep();
}


bool IntroDemoLoopMode::handleEvent(const SDL_Event& event) {
  if (!ui::isButtonPress(event)) {
    return false;
  }

  return base::match(mSteps[mCurrentStep],
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
      return true;
    },

    [&](auto&& state) {
      mContext.mpScriptRunner->handleEvent(event);
      return
        mContext.mpScriptRunner->hasFinishedExecution() &&
        mContext.mpScriptRunner->result()->mTerminationType ==
          ui::DukeScriptRunner::ScriptTerminationType::AbortedByUser;
    });
}


std::unique_ptr<GameMode> IntroDemoLoopMode::updateAndRender(
  const engine::TimeDelta dt,
  const std::vector<SDL_Event>& events
) {
  for (const auto& event : events) {
    const auto shouldQuit = handleEvent(event);
    if (shouldQuit) {
      updateCurrentStep(0.0);
      mContext.mpServiceProvider->fadeOutScreen();
      return std::make_unique<MenuMode>(mContext);
    }
  }

  updateCurrentStep(dt);

  if (isCurrentStepFinished()) {
    advanceToNextStep();
  }

  return {};
}


void IntroDemoLoopMode::startCurrentStep() {
  using std::begin;
  using std::end;

  base::match(mSteps[mCurrentStep],
    [this](Story& state) {
      runScript(mContext, "&Story");
    },

    [this](HypeScreen& state) {
      runScript(mContext, "HYPE");
    },

    [this](Credits& state) {
      auto creditsScript = mContext.mpScripts->at("&Credits");
      creditsScript.emplace_back(data::script::Delay{700});

      // The credits screen is shown twice as long in the registered version.
      // This makes the timing equivalent between the versions, only that the
      // shareware version will switch to the order info screen after half the
      // time has elapsed.
      //
      // Consequently, we always insert two 700 tick delays, but only insert
      // the order info script commands if we're running the shareware version.
      if (mContext.mpServiceProvider->isSharewareVersion()) {
        const auto orderInfoScript = mContext.mpScripts->at("Q_ORDER");
        creditsScript.insert(
          end(creditsScript), begin(orderInfoScript), end(orderInfoScript));
      }

      creditsScript.emplace_back(data::script::Delay{700});

      mContext.mpScriptRunner->executeScript(creditsScript);
    },

    [](auto&& state) {
      state.start();
    });
}


void IntroDemoLoopMode::updateCurrentStep(const engine::TimeDelta dt) {
  std::visit(
    [&](auto&& state) {
      if constexpr (
        std::is_base_of_v<ScriptedStep, std::decay_t<decltype(state)>>
      ) {
        mContext.mpScriptRunner->updateAndRender(dt);
      } else {
        state.updateAndRender(dt);
      }
    },
    mSteps[mCurrentStep]);
}


bool IntroDemoLoopMode::isCurrentStepFinished() const {
  return std::visit(
    [&](auto&& state) {
      if constexpr (
        std::is_base_of_v<ScriptedStep, std::decay_t<decltype(state)>>
      ) {
        return mContext.mpScriptRunner->hasFinishedExecution();
      } else {
        return state.isFinished();
      }
    },
    mSteps[mCurrentStep]);
}


void IntroDemoLoopMode::advanceToNextStep() {
  const auto isOneTimeStep =
    std::holds_alternative<Story>(mSteps[mCurrentStep]) ||
    std::holds_alternative<HypeScreen>(mSteps[mCurrentStep]);
  if (isOneTimeStep) {
    mSteps.erase(next(begin(mSteps), mCurrentStep));
  } else {
    ++mCurrentStep;

    if (mCurrentStep >= mSteps.size()) {
      mCurrentStep = 0;
    }
  }

  startCurrentStep();
}

}
