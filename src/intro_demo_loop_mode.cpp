#include "intro_demo_loop_mode.hpp"

#include "loader/resource_loader.hpp"


namespace rigel {

namespace {

struct ScriptExecutionStage {
  ui::DukeScriptRunner* mpScriptRunner;
  data::script::Script mScript;
};


void startStage(ScriptExecutionStage& stage) {
  stage.mpScriptRunner->executeScript(stage.mScript);
}


void updateStage(ScriptExecutionStage& stage, const engine::TimeDelta dt) {
  stage.mpScriptRunner->updateAndRender(dt);
}


bool isStageFinished(const ScriptExecutionStage& stage) {
  return stage.mpScriptRunner->hasFinishedExecution();
}


bool canStageHandleEvents(const ScriptExecutionStage&) {
  return true;
}


void forwardEventToStage(
  const ScriptExecutionStage& stage,
  const SDL_Event& event
) {
  stage.mpScriptRunner->handleEvent(event);
}

}


IntroDemoLoopMode::IntroDemoLoopMode(
  Context context,
  const bool isDuringGameStartup
)
  : mpServiceProvider(context.mpServiceProvider)
  , mFirstRunIncludedStoryAnimation(isDuringGameStartup)
  , mScriptRunner(context)
  , mScripts(context.mpResources->loadScriptBundle("TEXT.MNI"))
  , mCurrentStage(isDuringGameStartup ? 0 : 1)
{
  mStages.emplace_back(ui::ApogeeLogo(context));
  mStages.emplace_back(ui::IntroMovie(context));
  if (isDuringGameStartup) {
    mStages.emplace_back(ScriptExecutionStage{
      &mScriptRunner,
      mScripts["&Story"]});
  }

  auto creditsScript = mScripts["&Credits"];
  creditsScript.emplace_back(data::script::Delay{700});
  mStages.emplace_back(ScriptExecutionStage{
    &mScriptRunner,
    std::move(creditsScript)});

  // The credits screen is shown twice as long in the registered version. This
  // makes the timing equivalent between the versions, only that the shareware
  // version will switch to the order info screen after half the time has
  // elapsed.
  //
  // Consequently, we always insert two 700 tick delays, but only insert the
  // order info script commands if we're running the shareware version.
  auto orderInfoScript = data::script::Script{};
  if (context.mpServiceProvider->isShareWareVersion()) {
    orderInfoScript = mScripts["Q_ORDER"];
  }
  orderInfoScript.emplace_back(data::script::Delay{700});
  mStages.emplace_back(ScriptExecutionStage{
    &mScriptRunner,
    std::move(orderInfoScript)});

  startStage(mStages[mCurrentStage]);
}


void IntroDemoLoopMode::handleEvent(const SDL_Event& event) {
  if (event.type != SDL_KEYDOWN) {
    return;
  }

  if (mCurrentStage == 0) {
    // Pressing any key on the Apogee Logo skips forward to the intro movie
    mpServiceProvider->fadeOutScreen();
    mCurrentStage = 1;

    startStage(mStages[mCurrentStage]);
    updateStage(mStages[mCurrentStage], 0);
    mpServiceProvider->fadeInScreen();
  } else {
    auto& currentStage = mStages[mCurrentStage];

    if (
      event.key.keysym.sym == SDLK_ESCAPE
      || !canStageHandleEvents(currentStage)
    ) {
      mpServiceProvider->scheduleEnterMainMenu();
    } else {
      forwardEventToStage(currentStage, event);
    }
  }
}


void IntroDemoLoopMode::updateAndRender(const engine::TimeDelta dt) {
  updateStage(mStages[mCurrentStage], dt);

  if (isStageFinished(mStages[mCurrentStage])) {
    ++mCurrentStage;

    if (mCurrentStage >= mStages.size()) {
      mCurrentStage = 0;

      if (mFirstRunIncludedStoryAnimation) {
        const auto storyStageIter = mStages.cbegin() + 2;
        mStages.erase(storyStageIter);
        mFirstRunIncludedStoryAnimation = false;
      }
    }

    startStage(mStages[mCurrentStage]);
  }
}

}
