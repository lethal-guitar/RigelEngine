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

#include "actors.h"
#include "game.h"


/*******************************************************************************

Sprite system

This file contains higher level sprite drawing code, which builds upon the
low-level routines in gfx.asm. Sprite loading and some support functions like
collision detection (sprite against sprite) is also in here.

*******************************************************************************/


/** Test if two sprites are touching (intersecting)
 *
 * Returns true if the bounding box for the 1st sprite intersects the 2nd
 * sprite's bounding box, false otherwise.
 * The bounding box is defined by the dimensions of each sprite's graphical
 * data. The only exception to this is the player, which is handled specially
 * in this function to make the hitbox a little smaller for certain animation
 * frames. This is mainly to make it so that the weapon which protrudes from
 * Duke's body doesn't cause him to take damage when touching an enemy/hazard.
 * This special logic only applies if the *2nd* sprite's actor id is Duke's.
 */
bool AreSpritesTouching(
  Context* ctx,
  word id1,
  word frame1,
  word x1,
  word y1,
  word id2,
  word frame2,
  word x2,
  word y2)
{
  register word height1;
  word width1;
  word offset1;
  register word height2;
  word width2;
  word offset2;

  // Load the relevant meta data for both sprites
  offset1 = (word)(ctx->gfxActorInfoData[id1] + (frame1 << 3));
  x1 += AINFO_X_OFFSET(offset1);
  y1 += AINFO_Y_OFFSET(offset1);
  height1 = AINFO_HEIGHT(offset1);
  width1 = AINFO_WIDTH(offset1);

  offset2 = (word)(ctx->gfxActorInfoData[id2] + (frame2 << 3));
  x2 += AINFO_X_OFFSET(offset2);
  y2 += AINFO_Y_OFFSET(offset2);
  height2 = AINFO_HEIGHT(offset2);
  width2 = AINFO_WIDTH(offset2);

  // If the 2nd sprite is the player, do some hitbox adjustment. For everything
  // else in the game, the hitbox is always identical to the physical dimensions
  // of the sprite graphic. But the player is treated specially.
  if (id2 == ACT_DUKE_L)
  {
    // When looking up (frame 17), we don't want Duke's protruding weapon to
    // participate in collision detection. When crouching (frame 34), Duke's
    // head is also excluded from collision detection. I'm not sure if that's
    // intentional or not, as it looks a bit odd for enemy shots etc. to fly
    // through Duke's head without causing any harm..
    if (frame2 == 17 || frame2 == 34)
    {
      height2--;
    }

    // For animation frames where Duke's weapon protrudes to the left, we want
    // to exclude it from collision detection.
    if (
      frame2 < 9 || frame2 == 17 || frame2 == 18 || frame2 == 20 ||
      frame2 == 27 || frame2 == 28 || frame2 == 34)
    {
      width2--;
      x2++;
    }
  }
  else if (id2 == ACT_DUKE_R)
  {
    // Same as above
    if (frame2 == 17 || frame2 == 34)
    {
      height2--;
    }

    // Same as above, except that we don't need to adjust X for the right-facing
    // version
    if (
      frame2 < 9 || frame2 == 17 || frame2 == 18 || frame2 == 20 ||
      frame2 == 27 || frame2 == 28 || frame2 == 34)
    {
      width2--;
    }
  }

  // I'm not quite sure what this is meant to accomplish.. It makes it so that
  // a sprite which is outside of the map (horizontally) will have a hitbox
  // covering the entire width of the map, which seems odd. As far as I'm
  // aware, this case never occurs in the shipping game.
  if (x1 > ctx->mapWidth)
  {
    width1 = x1 + width1;
    x1 = 0;
  }

  // Now we can do the actual intersection test.
  if (
    ((x2 <= x1 && x2 + width2 > x1) || (x2 >= x1 && x1 + width1 > x2)) &&
    ((y1 - height1 < y2 && y2 <= y1) || (y2 - height2 < y1 && y1 <= y2)))
  {
    return true;
  }

  return false;
}


/** Test if a sprite is partially/fully visible (inside the viewport) */
bool IsSpriteOnScreen(Context* ctx, word id, word frame, word x, word y)
{
  register word width;
  register word height;
  word offset = (word)(ctx->gfxActorInfoData[id] + (frame << 3));

  x += AINFO_X_OFFSET(offset);
  y += AINFO_Y_OFFSET(offset);
  height = AINFO_HEIGHT(offset);
  width = AINFO_WIDTH(offset);

  if (
    // left edge on screen?
    (ctx->gmCameraPosX < x && x < ctx->gmCameraPosX + VIEWPORT_WIDTH) ||

    // right edge on screen?
    (ctx->gmCameraPosX >= x && x + width > ctx->gmCameraPosX))
  {
    // top edge on screen?
    if ((y - height + 1 < ctx->gmCameraPosY + ctx->mapViewportHeight &&
         y >= ctx->gmCameraPosY + ctx->mapViewportHeight))
    {
      return true;
    }
    // bottom edge on screen?
    else if (
      y >= ctx->gmCameraPosY && y < ctx->gmCameraPosY + ctx->mapViewportHeight)
    {
      return true;
    }
  }

  return false;
}


/** Play given sound if given actor is on screen */
void PlaySoundIfOnScreen(Context* ctx, word handle, byte soundId)
{
  ActorState* actor = ctx->gmActorStates + handle;

  // [NOTE] This could've been simplified by using IsActorOnScreen
  if (IsSpriteOnScreen(ctx, actor->id, actor->frame, actor->x, actor->y))
  {
    PlaySound(ctx, soundId);
  }
}


/** Convenience wrapper for IsSpriteOnScreen, for actors */
bool IsActorOnScreen(Context* ctx, word handle)
{
  ActorState* actor = ctx->gmActorStates + handle;

  return IsSpriteOnScreen(ctx, actor->id, actor->frame, actor->x, actor->y);
}
