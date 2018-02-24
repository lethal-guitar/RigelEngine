#pragma once

#include "loader/duke_script_loader.hpp"
#include "ui/apogee_logo.hpp"
#include "ui/intro_movie.hpp"
#include "game_mode.hpp"
#include "mode_stage.hpp"

#include <memory>


namespace rigel {

namespace ui { class DukeScriptRunner; }


/** Implements the intro/credits/demo loop
 *
 * This is the non-interactive "demo" mode of Duke Nukem II. It keeps
 * repeating the following sequence until any key is pressed:
 *
 *   Intro movie -> Credits -> Ordering Info (if ShareWare) -> in-game demos
 *     -> Apogee Logo
 *
 * This mode is entered when the user sits on the main menu for a certain
 * period of time without giving any input. It's also used for the game's
 * start, although it then starts on the Apogee Logo and includes the story
 * cutscene/animation.
 */
class IntroDemoLoopMode : public GameMode {
public:
  /** Construct an IntroDemoLoopMode instance
   *
   * When the game starts, the behavior is slightly different from the normal
   * intro/demo loop: The Apogee Logo is shown first, and the story cutscene
   * is shown after the intro movie.
   * Normally, the Apogee Logo comes last, and the story is not shown.
   *
   * The boolean argument `isDuringGameStartup` controls this behavior
   * accordingly.
   */
  IntroDemoLoopMode(Context context, bool isDuringGameStartup);

  void handleEvent(const SDL_Event& event) override;
  void updateAndRender(engine::TimeDelta dt) override;

private:

private:
  IGameServiceProvider* mpServiceProvider;
  bool mFirstRunIncludedStoryAnimation;

  ui::DukeScriptRunner* mpScriptRunner;
  loader::ScriptBundle mScripts;

  std::vector<ModeStage> mStages;
  std::size_t mCurrentStage;
};

}
