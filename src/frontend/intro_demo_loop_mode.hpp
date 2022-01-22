#pragma once

#include "assets/duke_script_loader.hpp"
#include "common/game_mode.hpp"
#include "ui/apogee_logo.hpp"
#include "ui/intro_movie.hpp"

#include <memory>
#include <variant>


namespace rigel
{

namespace ui
{
class DukeScriptRunner;
}


/** Implements the intro/credits/demo loop
 *
 * This is the non-interactive "demo" mode of Duke Nukem II. It keeps
 * repeating the following sequence until any key is pressed:
 *
 *   Intro movie -> Credits -> Ordering Info (if Shareware) -> in-game demos
 *     -> Apogee Logo
 *
 * This mode is entered when the user sits on the main menu for a certain
 * period of time without giving any input. It's also used for the game's
 * start, although it then starts on the Apogee Logo and includes the story
 * cutscene/animation.
 */
class IntroDemoLoopMode : public GameMode
{
public:
  enum class Type
  {
    Regular,
    DuringGameStart,
    AtFirstLaunch
  };

  /** Construct an IntroDemoLoopMode instance
   *
   * When the game starts, the behavior is slightly different from the normal
   * intro/demo loop: The Apogee Logo is shown first, and the story cutscene
   * is shown after the intro movie.
   * Normally, the Apogee Logo comes last, and the story is not shown.
   */
  IntroDemoLoopMode(Context context, Type type);

  std::unique_ptr<GameMode> updateAndRender(
    engine::TimeDelta dt,
    const std::vector<SDL_Event>& events) override;

private:
  struct ScriptedStep
  {
  };
  struct Credits : ScriptedStep
  {
  };
  struct HypeScreen : ScriptedStep
  {
  };
  struct Story : ScriptedStep
  {
  };

  using Step =
    std::variant<ui::ApogeeLogo, ui::IntroMovie, Story, HypeScreen, Credits>;

  bool handleEvent(const SDL_Event& event);
  void startCurrentStep();
  void updateCurrentStep(engine::TimeDelta dt);
  bool isCurrentStepFinished() const;
  void advanceToNextStep();

  Context mContext;
  std::vector<Step> mSteps;
  std::size_t mCurrentStep = 0;
};

} // namespace rigel
