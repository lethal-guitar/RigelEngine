/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 * This project is based on disassembly of NUKEM2.EXE from the game
 * Duke Nukem II, Copyright (C) 1993 Apogee Software, Ltd.
 *
 * Some parts of the code are based on or have been adapted from the Cosmore
 * project, Copyright (c) 2020-2022 Scott Smitelli.
 * See LICENSE_Cosmore file at the root of the repository, or refer to
 * https://github.com/smitelli/cosmore/blob/master/LICENSE.
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*******************************************************************************

Game logic, part 1: Dynamic level geometry (moving map parts)

*******************************************************************************/


/** Update moving map parts and shootable walls
 *
 * This function implements the control logic for the various types of dynamic
 * level geometry found in the game: Falling rocks, crumbling platforms,
 * destructible walls, unlockable doors etc.
 * These moving map parts are implemented via direct manipulation of the map
 * data in memory. Consequently, this function doesn't do any drawing/rendering.
 * The moving parts of the map are just tiles like any others, and thus they're
 * drawn by the regular map drawing code in UpdateAndDrawGame() in game3.c.
 */
void UpdateMovingMapParts(void)
{
  register word left;
  register word i;
  word top;
  word right;
  word bottom;
  MovingMapPartState* state;
  bool roundTwo;

  /*
  The logic here is a bit hard to follow, because the different types of
  dynamic geometry are all mixed together instead of having dedicated code
  paths. And for some reason, a different numbering scheme is used for the
  types than what's assigned to the trigger actors.
  Act_MovingMapPartTrigger() (in actors.c) sets the type to the actor's var2
  plus 98, except if var2 is 0. Overview:

  | var2 | type | behavior
  | ---- | ---- | --------
  |    0 |    0 | fall, then sink into ground with fire effects
  |    1 |   99 | shootable wall
  |    2 |  100 | fall, then sink into ground
  |    3 |  101 | fall, then explode
  |    4 |  102 | fall, wait if on solid ground
  |    5 |  103 | fall, wait if on solid ground
  |    6 |  104 | wait if on solid ground, then fall, then explode
  |    8 |  106 | like 102, but plays sound when starting out on solid ground

  Act_MovingMapPartTrigger() adds some additional behavior, like waiting
  for an earthquake, waiting a set delay, or waiting for a door to be unlocked.
  */

  for (i = 0; i < gmNumMovingMapParts; i++)
  {
    roundTwo = false;

    state = gmMovingMapParts + i;

    // Skip deactivated/invalid types
    if (state->type != 0 && state->type < 99) { continue; }

    if (state->type == 99) // shootable wall
    {
      if (FindPlayerShotInRect(
        state->left - 1, state->top - 2, state->right + 2, state->bottom + 1))
      {
        // Deactivate this state object (type 1 is skipped by the check above)
        state->type = 1;

        FLASH_SCREEN(SFC_WHITE);

        Map_DestroySection(
          state->left, state->top, state->right, state->bottom);
      }
    }
    else if (
      (state->type == 102 || state->type == 103 || state->type == 104) &&
      (HAS_TILE_ATTRIBUTE(
        Map_GetTile(state->left, state->bottom + 1), TA_SOLID_TOP) ||
      HAS_TILE_ATTRIBUTE(
        Map_GetTile(state->right, state->bottom + 1), TA_SOLID_TOP)))
    {
      // Types 102, 103, and 104 don't do anything as long as there's solid
      // ground below.
      continue;
    }
    else
    {
      // Type 104 waits while on solid ground. As soon as it starts falling,
      // turn it to type 105, which makes it explode when it reaches the ground
      // again.
      if (state->type == 104)
      {
        state->type = 105;
      }

doMovement:
      left = state->left;
      top = state->top;
      right = state->right;
      bottom = state->bottom;

      // Are we currently on solid ground? If we reach this code path with a
      // type of 102, 103, or 104, it means there was no solid ground before.
      if (
        HAS_TILE_ATTRIBUTE(
          Map_GetTile(state->left, state->bottom + 1), TA_SOLID_TOP) ||
        HAS_TILE_ATTRIBUTE(
          Map_GetTile(state->right, state->bottom + 1), TA_SOLID_TOP))
      {
        // Type 106 is turned into 102 once it reaches solid ground. The effect
        // of this is that map parts with type 106 that start out on solid
        // ground trigger the "ground reached" sound & screen shake effect as
        // soon as they are processed for the first time, which is not the case
        // for type 102 or 103 (since those go through the code path above where
        // we do a `continue` as long as there's solid ground below).
        if (state->type == 106)
        {
          state->type = 102;
        }

        // Types 102 and 103 play a sound and shake the screen when
        // reaching the ground
        if (state->type == 102 || state->type == 103)
        {
          PlaySound(SND_ROCK_LANDING);
          SHAKE_SCREEN(7);
        }
        // 101 and 105 explode when reaching the ground.
        else if (state->type == 101 || state->type == 105)
        {
          state->type = 1; // deactivate
          Map_DestroySection(left, top, right, bottom);
        }
        else // all other types sink into the ground
        {
          word x;

          // Add a "burning up" effect for type 0
          if (state->type == 0)
          {
            SHAKE_SCREEN(2);
            SpawnEffect(
              ACT_FLAME_FX,
              left + (int)RandomNumber() % (right - left),
              bottom + 1,
              EM_RISE_UP,
              0);
            PlaySound(SND_HAMMER_SMASH);
          }

          // Sink into the ground. By skipping the bottom row in the move,
          // it gets overwritten by the tiles above it.
          Map_MoveSection(left, top, right, bottom - 1, 1);
          top = state->top++;

          if (top == bottom) // Sinking complete?
          {
            // Erase the last row of tiles
            for (x = left; x <= right; x++)
            {
              Map_SetTile(0, x, bottom);
            }

            PlaySound(SND_ROCK_LANDING);
            gmMovingMapParts[i].type = 1; // deactivate
          }
        }
      }
      else // no solid ground below
      {
        // Fall down
        Map_MoveSection(left, top, right, bottom, 1);
        state->top++;
        state->bottom++;

        // For types 102/103, check if ground reached. If so, play a sound
        // and shake the screen.
        // [NOTE] This seems redundant - the same thing happens if we do the
        // goto (see above), so this entire if-statement could be removed, and
        // just the code in the else-block would be left.
        if (
          (state->type == 102 || state->type == 103) &&
          (HAS_TILE_ATTRIBUTE(
            Map_GetTile(state->left, state->bottom + 1), TA_SOLID_TOP) ||
          HAS_TILE_ATTRIBUTE(
            Map_GetTile(state->right, state->bottom + 1), TA_SOLID_TOP)))
        {
          PlaySound(SND_ROCK_LANDING);
          SHAKE_SCREEN(7);
        }
        else
        {
          // Do it all a 2nd time to achieve a falling speed of 2 units per
          // frame
          if (!roundTwo)
          {
            roundTwo = true;
            goto doMovement;
          }
        }
      }
    }
  }
}
