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

Sprite system

This file contains higher level sprite drawing code, which builds upon the
low-level routines in gfx.asm. Sprite loading and some support functions like
collision detection (sprite against sprite) is also in here.

*******************************************************************************/


/** Loads the actor info data (ACTRINFO.MNI) into memory
 *
 * This is a prerequisite for all the other sprite-related functions.
 * The graphics for the game's sprites are stored as raw data in a single big
 * file (ACTORS.MNI). The actor info data is necessary in order to know where
 * in that file to locate a specific sprite's graphics. It also contains meta-
 * data like the width and height of each sprite frame, which is also needed
 * for collision detection etc.
 *
 * In short: This function must be called before anything can be done that
 * involves sprites.
 */
void LoadActorInfo(void)
{
  word i;

  gfxActorInfoData = MM_PushChunk(GetAssetFileSize("ACTRINFO.MNI"), CT_COMMON);
  LoadAssetFile("ACTRINFO.MNI", gfxActorInfoData);

  for (i = 0; i < MM_MAX_NUM_CHUNKS; i++)
  {
    gfxLoadedSprites[i] = NULL;
  }
}


/** Load all sprite frames for the given actor ID
 *
 * The game loads and unloads sprites as needed, in order to conserve memory.
 * This function does the loading. It allocates memory, loads the data from the
 * file, and places it into the sprite data table (gfxLoadedSprites).
 * Unloading is done via the memory manager, by calling:
 *
 *   MM_PopChunks(CT_SPRITE);
 *
 * This automatically clears the corresponding entries in gfxLoadedSprites
 * as well.
 */
void pascal LoadSprite(int id)
{
  register word frame;
  register word firstFrameIndex;
  word height;
  word width;
  word offset;
  word numFrames;
  dword dataFileOffset;

  // If the sprite is already loaded, we have nothing to do.
  if (gfxLoadedSprites[FRAME_INDEX_MAP[id]])
  {
    return;
  }

  // The actor info data consists of two parts, an index table and the info
  // entries themselves. The index table is a list of word offsets into the
  // rest of the data, with each offset corresponding to the start of the info
  // data for the actor identified by that index.
  // The info data then consists of a series of 8-word long entries, with one
  // entry for each animation frame. The layout of the entries is described in
  // actors.h.
  //
  // Due to this layout, the first step for retrieving info data is always to
  // look up the starting offset in the index table. This is what we do here.
  offset = gfxActorInfoData[id];

  // Now with the offset at hand, we can read the number of frames this sprite
  // has.
  numFrames = AINFO_NUM_FRAMES(offset);

  for (frame = 0; frame < numFrames; frame++)
  {
    // Read dimensions, each frame can have a different size.
    // Unit is tiles.
    height = AINFO_HEIGHT(offset);
    width = AINFO_WIDTH(offset);

    // Read the offset (into ACTORS.MNI) where the frame's data starts
    dataFileOffset = AINFO_DATA_OFFSET(offset);

    // Destination index in gfxLoadedSprites for the first frame
    firstFrameIndex = FRAME_INDEX_MAP[id];

    // The sprite data is arranged into tiles (8x8 pixel blocks). Each block
    // occupies 40 bytes (5 bits per pixel, 64 pixels).
    // Thus, height * width * 40 is the total size in bytes of the frame.
    // Based on this, we can now allocate memory and load the data.
    *(gfxLoadedSprites + firstFrameIndex + frame) =
      MM_PushChunk(height * width * 40, CT_SPRITE);
    LoadAssetFilePart(
      "ACTORS.MNI",
      dataFileOffset,
      *(gfxLoadedSprites + firstFrameIndex + frame),
      height * width * 40);

    // Each actor info entry is 8 words in size, so this skips to the next
    // frame's info entry
    offset += 8;
  }
}


/** Load sprites for a range of actor ids */
void pascal LoadSpriteRange(int fromId, int toId)
{
  word i;

  for (i = fromId; i <= toId; i++)
  {
    LoadSprite(i);
  }
}


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
bool pascal AreSpritesTouching(
  word id1, word frame1, word x1, word y1,
  word id2, word frame2, word x2, word y2)
{
  register word height1;
  word width1;
  word offset1;
  register word height2;
  word width2;
  word offset2;

  // Load the relevant meta data for both sprites
  offset1 = gfxActorInfoData[id1] + (frame1 << 3);
  x1 += AINFO_X_OFFSET(offset1);
  y1 += AINFO_Y_OFFSET(offset1);
  height1 = AINFO_HEIGHT(offset1);
  width1 = AINFO_WIDTH(offset1);

  offset2 = gfxActorInfoData[id2] + (frame2 << 3);
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
  if (x1 > mapWidth)
  {
    width1 = x1 + width1;
    x1 = 0;
  }

  // Now we can do the actual intersection test.
  if ((
      (x2 <= x1 && x2 + width2 > x1) ||
      (x2 >= x1 && x1 + width1 > x2)
    ) && (
      (y1 - height1 < y2 && y2 <= y1) ||
      (y2 - height2 < y1 && y1 <= y2)
    ))
  {
    return true;
  }

  return false;
}


/** Test if a sprite is partially/fully visible (inside the viewport) */
bool pascal IsSpriteOnScreen(
  word id, word frame, word x, word y)
{
  register word width;
  register word height;
  word offset = gfxActorInfoData[id] + (frame << 3);

  x += AINFO_X_OFFSET(offset);
  y += AINFO_Y_OFFSET(offset);
  height = AINFO_HEIGHT(offset);
  width = AINFO_WIDTH(offset);

  if (
    // left edge on screen?
    (gmCameraPosX < x && x < gmCameraPosX + VIEWPORT_WIDTH) ||

    // right edge on screen?
    (gmCameraPosX >= x && x + width > gmCameraPosX))
  {
    // top edge on screen?
    if (
      (y - height + 1 < gmCameraPosY + mapViewportHeight &&
      y >= gmCameraPosY + mapViewportHeight))
    {
      return true;
    }
    // bottom edge on screen?
    else if (y >= gmCameraPosY && y < gmCameraPosY + mapViewportHeight)
    {
     return true;
    }
  }

  return false;
}


/** Play given sound if given actor is on screen */
void pascal PlaySoundIfOnScreen(word handle, byte soundId)
{
  ActorState* actor = gmActorStates + handle;

  // [NOTE] This could've been simplified by using IsActorOnScreen
  if (IsSpriteOnScreen(actor->id, actor->frame, actor->x, actor->y))
  {
    PlaySound(soundId);
  }
}


/** Convenience wrapper for IsSpriteOnScreen, for actors */
bool pascal IsActorOnScreen(word handle)
{
  ActorState* actor = gmActorStates + handle;

  return IsSpriteOnScreen(actor->id, actor->frame, actor->x, actor->y);
}


/** Draw sprite for an actor or other in-game object
 *
 * This function interprets the x & y coordinates as world coordinates,
 * meaning the camera position is taken into account when drawing the sprite.
 * Parts of the sprite which are at positions with foreground tiles are
 * skipped in order to make the sprite appear as if it is behind those tiles,
 * even though the tiles are actually drawn before drawing any sprites
 * (see UpdateAndDrawGame() in game2.c).
 * The different draw styles that can be used by actors are also implemented
 * by this function (see gamedefs.h).
 *
 * The graphical data for the specified sprite frame must've been loaded
 * before using this function.
 */
void pascal DrawActor(word id, word frame, word x, word y, word drawStyle)
{
  register word col;
  register word row;
  word offset;
  word lastCol;
  byte far* data;
  word mapTile;
  void (*drawFunc)(byte far*, word, word);

  if (drawStyle == DS_INVISIBLE) { return; }

  EGA_SET_DEFAULT_MODE();

  offset = gfxActorInfoData[id] + (frame << 3);

  x += AINFO_X_OFFSET(offset);
  col = x;
  y += AINFO_Y_OFFSET(offset);

  data = gfxLoadedSprites[FRAME_INDEX_MAP[id] + frame];

  if (drawStyle == DS_WHITEFLASH)
  {
    drawFunc = BlitMaskedTileWhiteFlash;
  }
  else if (drawStyle == DS_TRANSLUCENT)
  {
    drawFunc = BlitMaskedTileTranslucent;
  }
  else
  {
    drawFunc = BlitMaskedTile;
  }

  // Draw the sprite's tiles from top-left to bottom-right. The y coordinate
  // refers to the sprite's bottom row of tiles, hence we subtract height - 1
  // here to get to the top row.
  row = y - AINFO_HEIGHT(offset) + 1;

  // Cache the right-most column's coordinate for faster checking in the loop
  lastCol = x + AINFO_WIDTH(offset) - 1;

  for (;;)
  {
    // Only draw parts of the sprite that are visible on screen
    if (
      col >= gmCameraPosX && col < gmCameraPosX + VIEWPORT_WIDTH &&
      row >= gmCameraPosY && row < gmCameraPosY + mapViewportHeight)
    {
      mapTile = *(mapData + col + (row << mapWidthShift));

      // Skip drawing parts of the sprite that are meant to appear behind
      // foreground tiles. This doesn't apply for masked or composite tiles.
      // A draw style of DS_IN_FRONT also overrides this.
      if (
        !(gfxTilesetAttributes[mapTile >> 3] & TA_FOREGROUND) ||
        mapTile >= 8000 || // masked or composite tile
        drawStyle == DS_IN_FRONT)
      {
        drawFunc(data, col - gmCameraPosX + 1, row - gmCameraPosY + 1);
      }
    }

    // Masked sprite tiles are 40 bytes in size, so this gets us to the next
    // sprite tile in the source data.
    data += 40;

    if (col == lastCol)
    {
      if (row == y)
      {
        // Done!
        break;
      }

      // Next row
      col = x;
      row++;
    }
    else
    {
      col++;
    }
  }
}


/** Draw a 2x2 water area at the given location
 *
 * A water area changes the color of the pixels in the backbuffer.
 * An animStep of 0 applies the effect to the entire area. Other values
 * draw a water surface for the upper half of the area, which skips the
 * top-most pixels and draws a wave pattern based on the animStep.
 * Valid animStep values are between 1 and 4 (inclusive).
 */
void pascal DrawWaterArea(word left, word top, word animStep)
{
  register word x;
  register word y;
  void (*applyEffectFunc)(word, word);

  x = left;
  y = top;

  EGA_SET_DEFAULT_MODE();

  switch (animStep)
  {
    case 0:
      applyEffectFunc = ApplyWaterEffect;
      break;

    case 1:
      applyEffectFunc = ApplyWaterEffectWave0;
      break;

    case 3:
      applyEffectFunc = ApplyWaterEffectWave2;
      break;

    case 2:
    case 4:
      applyEffectFunc = ApplyWaterEffectWave1;
      break;
  }

  // Top left
  if (
    x >= gmCameraPosX && x < gmCameraPosX + VIEWPORT_WIDTH &&
    y >= gmCameraPosY && y < gmCameraPosY + mapViewportHeight)
  {
    applyEffectFunc(x - gmCameraPosX + 1, y - gmCameraPosY + 1);
  }

  x++;

  // Top right
  if (
    x >= gmCameraPosX && x < gmCameraPosX + VIEWPORT_WIDTH &&
    y >= gmCameraPosY && y < gmCameraPosY + mapViewportHeight)
  {
    applyEffectFunc(x - gmCameraPosX + 1, y - gmCameraPosY + 1);
  }

  y++;

  // Bottom right
  if (
    x >= gmCameraPosX && x < gmCameraPosX + VIEWPORT_WIDTH &&
    y >= gmCameraPosY && y < gmCameraPosY + mapViewportHeight)
  {
    ApplyWaterEffect(x - gmCameraPosX + 1, y - gmCameraPosY + 1);
  }

  x--;

  // Bottom left
  if (
    x >= gmCameraPosX && x < gmCameraPosX + VIEWPORT_WIDTH &&
    y >= gmCameraPosY && y < gmCameraPosY + mapViewportHeight)
  {
    ApplyWaterEffect(x - gmCameraPosX + 1, y - gmCameraPosY + 1);
  }
}


/** Load and draw a sprite at screen tile coordinates
 *
 * Unlike DrawActor, this function doesn't take the camera position into
 * account, nor does it consider foreground tiles. The given position is
 * relative to the top-left of the screen.
 * This function is suitable for drawing sprite-based graphics outside of
 * gameplay, i.e. in menus/UIs.
 *
 * Also unlike DrawActor, the graphical data needed to draw the
 * sprite is automatically loaded and unloaded (enough free memory
 * needs to be available). This behavior can be disabled by setting the most
 * significant bit in the actor ID, i.e. OR-ing with 0x8000.
 *
 * [HACK] It seems a bit hacky to shoehorn the "don't load" flag into the ID
 * like that. A dedicated boolean argument would be much clearer. I suspect
 * that the flag was added at a later point when the function was already in
 * use in several places, and the devs didn't feel like touching existing
 * call sites in order to add another parameter.
 */
void pascal DrawSprite(word id, word frame, word x, word y)
{
  register int col;
  register int row;
  word offset;
  word height;
  word width;
  word dontLoadData = id & 0x8000;
  word framesStart;
  bool dataWasLoaded = false;
  byte far* data;

  if (dontLoadData)
  {
    id &= 0x7FFF;
  }

  framesStart = FRAME_INDEX_MAP[id];

  EGA_SET_DEFAULT_MODE();

  offset = gfxActorInfoData[id] + (frame << 3);

  x += AINFO_X_OFFSET(offset);
  col = x;
  y += AINFO_Y_OFFSET(offset);
  height = AINFO_HEIGHT(offset);
  width = AINFO_WIDTH(offset);

  // [NOTE] Moving this statement after the following if-statement would
  // avoid the need to reassign the data pointer inside the if-statement.
  data = gfxLoadedSprites[framesStart + frame];

  if (dontLoadData == false && !gfxLoadedSprites[framesStart])
  {
    LoadSprite(id);
    data = gfxLoadedSprites[framesStart + frame];
    dataWasLoaded = true;
  }

  // Draw the sprite's tiles from top-left to bottom-right. The y coordinate
  // refers to the sprite's bottom row of tiles, hence we subtract height - 1
  // here to get to the top row.
  row = y - height + 1;

  for (;;)
  {
    BlitMaskedTile(data, col - 1, row);

    // Masked sprite tiles are 40 bytes in size, so this gets us to the next
    // sprite tile in the source data.
    data += 40;

    // Have we reached the right-most column?
    if (col == x + width - 1)
    {
      if (row == y)
      {
        // We're done, deallocate data if we allocated it
        if (dataWasLoaded && !dontLoadData)
        {
          MM_PopChunks(CT_SPRITE);
        }
        break;
      }

      // Next row
      col = x;
      row++;
    }
    else
    {
      col++;
    }
  }
}


/** Draws the Duke3D teaser text sprite at (roughly) the given position
 *
 * The x value is given in tiles, whereas y is given in pixels.  Y refers to
 * the top of the sprite, but will be offset slightly (see comments in the
 * function body).
 *
 * [NOTE] This function seems like it was very quickly cobbled together,
 * possibly late at night. It's mostly copy-pasted from DrawSprite, with some
 * adjustments to allow for pixel-wise positioning on the Y axis. But the
 * adjustments aren't fully implemented, making this function work somewhat
 * incorrectly. Ultimately, it doesn't really matter though. The teaser screen
 * works perfectly well and is only displayed briefly. No regular user will
 * notice that the logo is a few pixels off from where the code says it should
 * be.
 * Most likely, the developers added the teaser screen fairly late in
 * development, as a cool idea that they wanted to include but didn't have much
 * time for.
 */
void pascal DrawDuke3dTeaserText(word x, word y)
{
  register int col;
  register int yPos;
  word offset;
  word height;
  word width;
  word framesStart;
  word yOffset;
  byte far* data;

  framesStart = FRAME_INDEX_MAP[ACT_DUKE_3D_TEASER_TEXT];

  EGA_SET_DEFAULT_MODE();

  offset = gfxActorInfoData[ACT_DUKE_3D_TEASER_TEXT];

  x += AINFO_X_OFFSET(offset);
  col = x;

  // [BUG] The y offset should be multiplied by 8, since y is given in pixels
  // for this function.  The Duke3D teaser sprite doesn't have a y offset so it
  // doesn't matter, but then why apply the offset at all?
  y += AINFO_Y_OFFSET(offset);

  height = AINFO_HEIGHT(offset);
  width = AINFO_WIDTH(offset);

  data = gfxLoadedSprites[framesStart];

  // [BUG] height should be multiplied by 8 here, since y is given in pixels for
  // this function. This causes the sprite to appear offset from the given y
  // coordinate by 6 pixels, since the graphic is 7 tiles high.
  yPos = y - height + 1;
  yOffset = 0;

  for (;;)
  {
    // [NOTE] Using two variables here for the y position is a little
    // convoluted. It would seem easier to initialize yPos to y and then simply
    // add 8 to yPos after each row.  To check if all rows have been drawn, we
    // could test if y == y + height * 8. A multiplication by 8 can be expressed
    // as a left-shift by 3, so performance shouldn't be a concern (and either
    // way, this function is not used frequently enough to really pose any
    // performance issues).
    BlitMaskedTile_FlexibleY(data, col - 1, yPos + yOffset);
    data += 40;

    if (col == x + width - 1)
    {
      if (yPos == y)
      {
        break;
      }

      col = x;
      yPos++;
      yOffset += 7;
    }
    else
    {
      col++;
    }
  }
}


/** Draw a single color plane of the specified large font character
 *
 * See DrawColorizedChar() in draw3.c.
 */
void pascal DrawFontSprite(word charIndex, word x, word y, word plane)
{
  byte far* data;
  word firstFrame = FRAME_INDEX_MAP[ACT_MENU_FONT_GRAYSCALE];

  EGA_SET_DEFAULT_MODE();

  data = gfxLoadedSprites[firstFrame + charIndex];

  // Characters in the sprite-based font are 1x2 tiles in size, we draw both
  // of the vertical tiles here. Unlike regular masked sprites, the font is
  // monochromatic with only one mask plane and one color bit plane.
  // Therefore, a single font sprite tile is 16 bytes rather than 40 bytes.
  BlitFontTile(data,      x - 1, y,     plane);
  BlitFontTile(data + 16, x - 1, y + 1, plane);
}
