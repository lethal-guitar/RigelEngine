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

Game logic, part 2: Top-level frame update, auxiliary game objects

This file defines the logic for "auxiliary" game objects that aren't represented
by actors: Effects, player shots, tile debris.

It also contains the top-level frame update function, which includes all the
map and background drawing code, and a few random building block functions.

*******************************************************************************/




/** Update backdrop state for parallax scrolling etc.
 *
 * Depending on the current level's backdrop settings, this function does
 * different things that cause the backdrop drawing code in UpdateAndDrawGame
 * to have the desired results.
 * It also handles the earthquake and reactor destruction event effects.
 */
void UpdateBackdrop(Context* ctx)
{
  byte random;

  //
  // Earthquake effect
  //
  if (ctx->mapHasEarthquake)
  {
    if (
      ctx->gmEarthquakeCountdown < ctx->gmEarthquakeThreshold &&
      ctx->gmEarthquakeCountdown != 0)
    {
      random = RandomNumber(ctx) & 0x3;
      ShowTutorial(ctx, TUT_EARTHQUAKE);

      if (random)
      {
        SHAKE_SCREEN(random);
      }
      else
      {
        PlaySound(ctx, SND_EARTHQUAKE + (random > 2 ? 1 : 0));
      }
    }

    if (!ctx->gmEarthquakeCountdown)
    {
      ctx->gmEarthquakeCountdown = RandomNumber(ctx);
      ctx->gmEarthquakeThreshold = RandomNumber(ctx) % 50;
    }

    ctx->gmEarthquakeCountdown--;
  }

  if (ctx->mapParallaxHorizontal)
  {
    // Reactor destruction event (backdrop flashes white every other frame)
    // update. The actual flashing happens in UpdateAndDrawGame() based on
    // the state of gmReactorDestructionStep, here we just update the step
    // variable, play sound effects, and show a message.
    if (ctx->bdUseSecondary && ctx->mapHasReactorDestructionEvent)
    {
      if (ctx->gmReactorDestructionStep < 14)
      {
        ctx->gmReactorDestructionStep++;

        if (ctx->gfxCurrentDisplayPage)
        {
          PlaySound(ctx, SND_BIG_EXPLOSION);
        }
      }

      if (ctx->gmReactorDestructionStep == 13)
      {
        ShowInGameMessage(ctx, MID_DESTROYED_EVERYTHING);
      }
    }
  }
}


/** Update game logic and draw game world
 *
 * This is the root function of the game logic. It's invoked once every
 * frame by RunInGameLoop() in main.c.
 * It advances the game world simulation by one step and draws the resulting
 * state of the world. This includes parallax background, map tiles, sprites,
 * particle effects etc.
 */
void pascal UpdateAndDrawGame(Context* ctx)
{
  if (ctx->gfxFlashScreen)
  {
    ctx->gfxCurrentDisplayPage = !ctx->gfxCurrentDisplayPage;
    ctx->gfxFlashScreen = false;
  }

  UpdatePlayer(ctx);
  UpdateBackdrop(ctx);
  UpdateMovingMapParts(ctx);
  UpdateAndDrawActors(ctx);
  UpdateAndDrawParticles(ctx);
  UpdateAndDrawPlayerShots(ctx);
  UpdateAndDrawEffects(ctx);
  UpdateAndDrawTileDebris(ctx);

  ctx->gfxCurrentDisplayPage = !ctx->gfxCurrentDisplayPage;
}


/** Test if sprite is colliding with the world/map data in given direction
 *
 * This function implements the game's world collision detection. Given a
 * bounding box (derived from actor ID and sprite frame), position, and
 * movement direction, it returns whether the sprite is colliding with the
 * world. Typically, the position given to this function is the position
 * that the actor/sprite wants to move to, not the current position.
 * Basically, this function tells you if the sprite would be _inside_ a wall
 * at the given location, not whether it's up against a wall.
 *
 * But wait, there's more! When checking for collision downwards, the
 * function also sets the global variable retConveyorBeltCheckResult to
 * indicate if the actor/sprite is currently on top of tiles with the
 * conveyor belt flag set, and which direction the conveyor belt is moving.
 *
 * And then there's some more special behavior when checking collision for the
 * player sprite: For checking upwards and downwards, the return value also
 * indicates whether there's a ladder or climbable pipe/vine etc.
 * For checking left and right, the collision detection checks for stair steps.
 * If the wall blocking the player is only one tile tall, the player is moved
 * up by one instead of indicating a collision. This is used a lot for the
 * game's version of sloped surfaces, which are actually stairs that are made
 * to look like a slope with the help of masked (partially transparent) tiles.
 */
int16_t pascal CheckWorldCollision(
  Context* ctx,
  word direction,
  word actorId,
  word frame,
  word x,
  word y)
{
  register int16_t i;
  register word height;
  word far* tileData;
  bool isPlayer = false;
  bool atStairStep = false;
  word width;
  word offset;
  int16_t bboxTop;
  word attributes;

  ctx->retConveyorBeltCheckResult = CB_NONE;

  offset = ctx->gfxActorInfoData[actorId] + (frame << 3);
  height = AINFO_HEIGHT(offset);
  width = AINFO_WIDTH(offset);

  // Adjust bounding box if we're dealing with the player sprite.  Similarly to
  // what's done in AreSpritesTouching(), the width and position are adjusted so
  // that Duke's protruding weapon doesn't participate in collision detection.
  if (actorId == ACT_DUKE_L)
  {
    isPlayer = true;

    if (ctx->plPosX == 0 && direction == MD_LEFT)
    {
      return CR_COLLISION;
    }

    if (frame == 0 || frame == 37)
    {
      width--;
      x++;
    }
  }
  else if (actorId == ACT_DUKE_R)
  {
    isPlayer = true;

    if (frame == 0)
    {
      width--;
    }
  }
  else
  {
    // Otherwise, if we're not checking the player, apply x/y offset.
    x += AINFO_X_OFFSET(offset);
    y += AINFO_Y_OFFSET(offset);
  }

  switch (direction)
  {
    case MD_PROJECTILE:
      // Projectile (player shot) collision detection works a little
      // differently, presumably for speed reasons. The code here checks
      // both the left and top edges of the sprite's bounding box for
      // collision. This means it's not necessary to specify a movement
      // direction, the check always works - but only for sprites which are
      // exactly one tile in one dimension, e.g. 1x4 or 4x1. This happens to
      // apply to almost all player shots, only the flame thrower and reactor
      // fire are exceptions. But both of those fly through walls and thus
      // don't need collision detection.
      //
      // Side-note: The reactor fire is not something the player can shoot,
      // it appears when the player destroys a reactor. But it is still
      // implemented as a player shot on the engine side.
      bboxTop = y - height + 1;

      // Top of the map is never considered solid
      if (bboxTop < 0 || y == 0)
      {
        return CR_NONE;
      }

      // Start at map tile underneath the sprite's top-left corner
      tileData = ctx->mapData + ((y - height + 1) << ctx->mapWidthShift) + x;

      // Check the top edge. This is the entire length of the sprite for
      // horizontal shots, and only the top-most tile for vertical shots.
      for (i = 0; i < width; i++)
      {
        attributes = *(ctx->gfxTilesetAttributes + (*(tileData + i) >> 3));

        // Treat composite tiles as not solid - and abort the entire check.
        if (*(tileData + i) & 0x8000)
        {
          return CR_NONE;
        }

        // If any of the checked tiles is solid in any direction, we have a
        // hit
        if (attributes & 0xF)
        {
          return CR_COLLISION;
        }
      }

      // Start at map tile underneath the sprite's bottom-left corner
      tileData = ctx->mapData + (y << ctx->mapWidthShift) + x;

      // Check left edge, this is the entire height of the sprite for vertical
      // shots, and only the left-most tile for horizontal shots
      for (i = 0; i < height; i++)
      {
        attributes = *(ctx->gfxTilesetAttributes + (*tileData >> 3));

        // Treat composite tiles as not solid - and abort the entire check.
        if (*tileData & 0x8000)
        {
          return CR_NONE;
        }

        // If any of the checked tiles is solid in any direction, we have a
        // hit
        if (attributes & 0xF)
        {
          return CR_COLLISION;
        }

        // Go up by one tile
        tileData -= ctx->mapWidth;
      }

      return CR_NONE;

    case MD_UP:
      bboxTop = y - height + 1;

      // Upper edge outside the map is never solid
      if (bboxTop < 0)
      {
        return CR_NONE;
      }

      // Start at map tile underneath top-left corner of the sprite
      tileData = ctx->mapData + ((y - height + 1) << ctx->mapWidthShift) + x;

      if (isPlayer && HAS_TILE_ATTRIBUTE(*(tileData + 1), TA_CLIMBABLE))
      {
        return CR_CLIMBABLE;
      }

      // Check top edge of the sprite
      for (i = 0; i < width; i++)
      {
        if (HAS_TILE_ATTRIBUTE(*(tileData + i), TA_SOLID_BOTTOM))
        {
          return CR_COLLISION;
        }
      }

      // Special logic for climbing ladders
      if (isPlayer)
      {
        if (HAS_TILE_ATTRIBUTE(*(tileData + 1), TA_LADDER))
        {
          return CR_LADDER;
        }

        if (ctx->inputMoveLeft || ctx->inputMoveRight)
        {
          // No-op
        }
        else
        {
          if (ctx->inputMoveUp && HAS_TILE_ATTRIBUTE(*tileData, TA_LADDER))
          {
            ctx->plPosX--;
            return CR_LADDER;
          }

          if (
            ctx->inputMoveUp && HAS_TILE_ATTRIBUTE(*(tileData + 2), TA_LADDER))
          {
            ctx->plPosX++;
            return CR_LADDER;
          }
        }
      }

      return CR_NONE;

    case MD_DOWN:
      // Start at map tile underneath sprite's bottom-left corner
      tileData = ctx->mapData + (y << ctx->mapWidthShift) + x;

      // Bottom edge outside the map is never solid
      if (y > ctx->mapBottom)
      {
        return CR_NONE;
      }

      // Check bottom edge of the sprite
      for (i = 0; i < width; i++)
      {
        // Conveyor belt checks
        if (HAS_TILE_ATTRIBUTE(*(tileData + i), TA_CONVEYOR_L))
        {
          ctx->retConveyorBeltCheckResult = CB_LEFT;
        }

        if (
          HAS_TILE_ATTRIBUTE(*(tileData + i), TA_CONVEYOR_R) &&
          (HAS_TILE_ATTRIBUTE(*(tileData + width - 1), TA_CONVEYOR_R) ||
           !HAS_TILE_ATTRIBUTE(*(tileData + width - 1), TA_SOLID_TOP)))
        {
          ctx->retConveyorBeltCheckResult = CB_RIGHT;
        }

        // Collision check
        if (HAS_TILE_ATTRIBUTE(*(tileData + i), TA_SOLID_TOP))
        {
          return CR_COLLISION;
        }
      }

      // Special logic for climbing ladders
      if (isPlayer && HAS_TILE_ATTRIBUTE(*(tileData + 1), TA_LADDER))
      {
        return CR_LADDER;
      }

      return CR_NONE;

    case MD_LEFT:
      bboxTop = y - height + 1;

      if (bboxTop < 0)
      {
        return CR_NONE;
      }

      // Left edge outside the map is always solid. This takes advantage of
      // unsigned wrap-around, so if x would be negative when treated as a
      // signed value, then it will be larger than ctx->mapWidth if treated as
      // unsigned.
      if (x > ctx->mapWidth)
      {
        return CR_COLLISION;
      }

      // Start at map tile underneath the sprite's bottom-left corner
      tileData = ctx->mapData + (y << ctx->mapWidthShift) + x;

      // Check the sprite's left edge
      for (i = 0; i < height; i++)
      {
        if (HAS_TILE_ATTRIBUTE(*tileData, TA_SOLID_RIGHT))
        {
          if (isPlayer && ctx->plState == PS_NORMAL)
          {
            atStairStep = true;

            // Stair stepping doesn't apply if any of the tiles above the stair
            // step are solid
            if (i)
            {
              return CR_COLLISION;
            }
          }
          else
          {
            return CR_COLLISION;
          }
        }

        // Go up by one map tile
        tileData -= ctx->mapWidth;
      }

      // When at a stair step, move the player up by one and report "no
      // collision". The player movement code will then move the player to the
      // left to make them actually stand on the stair step (see UpdatePlayer
      // in player.c).
      if (atStairStep)
      {
        ctx->plPosY--;
      }

      return CR_NONE;

    case MD_RIGHT:
      bboxTop = y - height + 1;

      if (bboxTop < 0)
      {
        return CR_NONE;
      }

      // Right edge outside the map is always solid
      if (x + width - 1 >= ctx->mapWidth)
      {
        return CR_COLLISION;
      }

      // Start at map tile underneath the sprite's bottom-right corner
      tileData = ctx->mapData + (y << ctx->mapWidthShift) + x + width - 1;

      // Check sprite's right edge
      for (i = 0; i < height; i++)
      {
        if (HAS_TILE_ATTRIBUTE(*tileData, TA_SOLID_LEFT))
        {
          if (isPlayer && ctx->plState == PS_NORMAL)
          {
            atStairStep = true;

            // Stair stepping doesn't apply if any of the tiles above the stair
            // step are solid
            if (i)
            {
              return CR_COLLISION;
            }
          }
          else
          {
            return CR_COLLISION;
          }
        }

        tileData -= ctx->mapWidth;
      }

      // When at a stair step, move the player up by one and report "no
      // collision". The player movement code will then move the player to the
      // right to make them actually stand on the stair step (see UpdatePlayer
      // in player.c).
      if (atStairStep)
      {
        ctx->plPosY--;
      }

      return CR_NONE;
  }

  return CR_NONE;
}


/** Remove all currently active effects and player shots */
void ResetEffectsAndPlayerShots(Context* ctx)
{
  register word i;

  for (i = 0; i < MAX_NUM_EFFECTS; i++)
  {
    ctx->gmEffectStates[i].active = 0;

    if (i < MAX_NUM_PLAYER_SHOTS)
    {
      ctx->gmPlayerShotStates[i].active = 0;
    }
  }
}


/** Erase map data and spawn flying debris for the specified region */
void pascal
  Map_DestroySection(Context* ctx, word left, word top, word right, word bottom)
{
  int16_t i;
  word x;
  word y;
  word tileValue;

  PlaySound(ctx, SND_BIG_EXPLOSION);

  right += 1;
  bottom += 1;

  // Set up state for flying tile debris
  ctx->gmExplodingSectionLeft = left;
  ctx->gmExplodingSectionTop = top;
  ctx->gmExplodingSectionRight = right;
  ctx->gmExplodingSectionBottom = bottom;
  ctx->gmExplodingSectionTicksElapsed = 1;

  // Spawn pieces of debris for each tile in the affected region, and erase
  // map data
  i = 0;

  for (y = top; y < bottom; y++)
  {
    for (x = left; x < right; x++)
    {
      tileValue = Map_GetTile(ctx, x, y);

      if (tileValue) // skip empty map cells
      {
        // Tile debris state is stored as a plain array of word values, not
        // structs. The data layout is:
        //
        // 0: word xVelocity;
        // 1: word tableIndex;
        // 2: word tileValue;
        // 3: word x
        // 4: word y

        ctx->gmTileDebrisStates[i + 0] = 3 - RandomNumber(ctx) % 6;
        ctx->gmTileDebrisStates[i + 1] = RandomNumber(ctx) % 5;
        ctx->gmTileDebrisStates[i + 2] = tileValue;
        ctx->gmTileDebrisStates[i + 3] = x - ctx->gmCameraPosX;
        ctx->gmTileDebrisStates[i + 4] = y - ctx->gmCameraPosY;

        // Advance to the start of the next tile debris state object
        i += 5;

        Map_SetTile(ctx, 0, x, y);
      }
    }
  }
}


/** Update and draw a currently active tile explosion */
void UpdateAndDrawTileDebris(Context* ctx)
{
  // [PERF] Missing `static` causes a copy operation here
  const int16_t Y_MOVEMENT[] = {-3, -3, -2, -2, -1, 0, 0, 1, 2, 2, 3};

  register word i;
  register word size;
  word far* debris;

  // If there's no flying tile debris right now, stop here.
  if (ctx->gmExplodingSectionTicksElapsed == 0)
  {
    return;
  }

  // size here is the number of word values we need to process. Each tile debris
  // piece occupies 5 words.
  size = (ctx->gmExplodingSectionRight - ctx->gmExplodingSectionLeft) *
    (ctx->gmExplodingSectionBottom - ctx->gmExplodingSectionTop) * 5;

  for (i = 0; i < size; i += 5)
  {
    // Tile debris state is stored as a plain array of word values, not structs.
    // The data layout is:
    //
    // 0: word xVelocity;
    // 1: word tableIndex;
    // 2: word tileValue;
    // 3: word x
    // 4: word y
    debris = ctx->gmTileDebrisStates + i;

    debris[3] += debris[0]; // debris->x += xVelocity
    debris[4] += Y_MOVEMENT[debris[1]]; // debris->y += Y_MOVEMENT[
                                        //   debris->tableIndex]

    // [BUG] The Y_MOVEMENT array only has 11 elements, but here we increment
    // the index up to a value of 12 (13th element). This causes an
    // out-of-bounds read for debris pieces which have reached index 11. The
    // next 4 bytes in memory produce the values 256 and 770 when interpreted
    // as two integers. Since these values are fairly large (possibly larger
    // than the level is tall), all debris pieces effectively disappear from
    // view after just a few frames.
    if (debris[1] < 13) // if (debris->tableIndex < 13)
    {
      debris[1]++; // debris->tableIndex++
    }

    // arguments are: tileValue, x, y
    DrawTileDebris(ctx, debris[2], debris[3], debris[4]);
  }

  // Advance the timer, until the maximum time is reached. At that point, we
  // set the tick counter to 0, which stops this function from doing anything.
  // The tile debris state remains, it will be overwritten the next time a map
  // section is destroyed.
  //
  // [NOTE] 80 seems excessively high, given that not a single tile debris
  // piece remains visible after just 11 frames.
  ctx->gmExplodingSectionTicksElapsed++;
  if (ctx->gmExplodingSectionTicksElapsed == 80)
  {
    ctx->gmExplodingSectionTicksElapsed = 0;
  }
}


/** Return whether effect with given actor ID should damage the player */
static bool pascal EffectIsDamaging(word actorId)
{
  switch (actorId)
  {
    case ACT_FLAME_THROWER_FIRE_R:
    case ACT_FLAME_THROWER_FIRE_L:
    case ACT_NUCLEAR_EXPLOSION:
    case ACT_FIRE_BOMB_FIRE:
    case ACT_HOVERBOT_TELEPORT_FX:
    case ACT_NUCLEAR_WASTE:
    case ACT_EYEBALL_PROJECTILE:
    case ACT_RIGELATIN_SOLDIER_SHOT:
      return true;
  }

  return false;
}


/** Spawn a new effect into the game world, if possible
 *
 * Does nothing if the maximum number of effects is already reached.
 */
bool pascal
  SpawnEffect(Context* ctx, word id, word x, word y, word type, word spawnDelay)
{
  register word i;
  register word offset = ctx->gfxActorInfoData[id];
  EffectState* state;
  word numFrames = AINFO_NUM_FRAMES(offset);

  // Search for a free slot in the effect states list
  for (i = 0; i < MAX_NUM_EFFECTS; i++)
  {
    if (ctx->gmEffectStates[i].active == 0)
    {
      // We found a slot, set it up
      state = ctx->gmEffectStates + i;

      // If we're spawning a fire bomb fire, only do it if there's solid ground
      // below. Return true to indicate that spawning failed.
      //
      // [NOTE] This feels like the wrong layer of abstraction to perform this
      // check. There are only two places in the entire code base which rely
      // on this behavior, and there's no reason why the check couldn't be done
      // there instead.
      if (
        id == ACT_FIRE_BOMB_FIRE &&
        !CheckWorldCollision(ctx, MD_DOWN, ACT_FIRE_BOMB_FIRE, 0, x, y + 1))
      {
        return true;
      }

      state->active = 1;
      state->id = id;
      state->framesToLive = numFrames + 1;
      state->x = x;
      state->y = y;
      state->type = type;
      state->movementStep = 0;
      state->spawnDelay = spawnDelay;
      break;
    }
  }

  return false;
}


/** Spawn multiple effects based on specification
 *
 * This is a convenience function for spawning multiple effects, usually
 * used for destruction effects. The specification should start with the
 * number of effects to spawn, followed by that many groups of 4 numbers.
 * Each group consists of: x offset, y offset, effect type, spawn delay.
 *
 * See game3.c for various examples of how this function is used.
 */
void pascal SpawnDestructionEffects(
  Context* ctx,
  word handle,
  int16_t* spec,
  word actorId)
{
  register ActorState* actor = ctx->gmActorStates + handle;
  word entriesLeft = *spec;
  spec++;

  while (entriesLeft--)
  {
    SpawnEffect(
      ctx, actorId, actor->x + spec[0], actor->y + spec[1], spec[2], spec[3]);
    spec += 4;
  }
}


/** Make effect spawn repeatedly over time
 *
 * This function doesn't directly spawn an effect. Instead, it creates a
 * "effect spawner" which will spawn multiple instances of the specified
 * sprite ID over the course of the next couple of frames. Each spawned
 * effect is randomly positioned within the bounding box specified by the
 * sourceId parameter. When using ACT_FLAME_FX as the spawned effect type,
 * this creates the impression of something going up in flames, hence the
 * name of this function.
 *
 * Does nothing if the maximum number of effects is already reached.
 */
void pascal
  SpawnBurnEffect(Context* ctx, word effectId, word sourceId, word x, word y)
{
  register word offset;
  register word i;
  EffectState* state;
  word height;
  word width;

  // The continually spawning effects should appear in an area corresponding
  // to the source sprite's bounding box, so we apply the x/y offset here.
  offset = ctx->gfxActorInfoData[sourceId];
  x += AINFO_X_OFFSET(offset);
  y += AINFO_Y_OFFSET(offset);

  // Search for an available slot
  for (i = 0; i < MAX_NUM_EFFECTS; i++)
  {
    if (ctx->gmEffectStates[i].active == 0)
    {
      // We found a free slot, set it up
      state = ctx->gmEffectStates + i;

      state->active = 18;
      state->id = sourceId;

      // The EffectState struct fields are repurposed with different meanings
      // for the EM_BURN_FX type. See UpdateAndDrawEffects().
      state->framesToLive = effectId;

      // Get height & width of the effect sprite
      offset = ctx->gfxActorInfoData[effectId];
      height = AINFO_HEIGHT(offset);
      width = AINFO_WIDTH(offset);

      // Set x and y so that an effect sprite spawned there will appear
      // centered at that location
      //
      // [BUG] height and width are swapped here. The game only ever
      // uses this function with sprites that are square in size, so
      // it doesn't make any difference.
      state->x = x - height / 2;
      state->y = y + width / 2;

      state->type = EM_BURN_FX;

      // Store height and width of the _source_ sprite, so that we can define
      // the spawn area
      offset = ctx->gfxActorInfoData[sourceId];
      height = AINFO_HEIGHT(offset);
      width = AINFO_WIDTH(offset);
      state->movementStep = height;
      state->spawnDelay = width;

      // All state set up, UpdateAndDrawEffects() can now process this effect
      return;
    }
  }

  // If we get here, then all slots are already occupied, and we fail
  // silently.
}


/** Update and draw all currently active effects */
void UpdateAndDrawEffects(Context* ctx)
{
  register EffectState* state;
  register int16_t j;
  word i;

  for (i = 0; i < MAX_NUM_EFFECTS; i++)
  {
    if (!ctx->gmEffectStates[i].active)
    {
      continue;
    }

    state = ctx->gmEffectStates + i;

    if (state->type == EM_SCORE_NUMBER)
    {
      // For score numbers, the spawnDelay field is used to keep track of
      // elapsed time.
      const byte SCORE_NUMBER_ANIMATION[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1};

      state->spawnDelay++;

      if (state->spawnDelay < 6)
      {
        state->y--;
      }
      else
      {
        if (state->spawnDelay > 20)
        {
          // The active field also doubles as animation frame table index.
          state->active++;

          if (state->spawnDelay > 34)
          {
            state->y--;
          }
        }
      }

      DrawActor(
        ctx,
        state->id,
        SCORE_NUMBER_ANIMATION[(state->active - 1) % 14],
        state->x,
        state->y,
        DS_NORMAL);

      if (state->spawnDelay == 60)
      {
        state->active = 0;
      }
    }
    else if (state->type == EM_BURN_FX)
    {
      if (state->active % 2)
      {
        // See SpawnBurnEffect(). spawnDelay is source sprite width,
        // movementStep is source sprite height.
        // So this spawns an effect somewhere within the source sprite's
        // bounding box, randomly placed.
        SpawnEffect(
          ctx,
          state->framesToLive, // ID to spawn
          state->x + (int16_t)RandomNumber(ctx) % state->spawnDelay,
          state->y - (int16_t)RandomNumber(ctx) % state->movementStep,
          EM_RISE_UP,
          0);
      }

      state->active--;
    }
    else // all other types of effects
    {
      if (state->type == EM_NONE || state->type == EM_RISE_UP)
      {
        // Delete effects that have disappeared from view
        if (!IsSpriteOnScreen(
              ctx, state->id, state->active - 1, state->x, state->y))
        {
          state->active = 0;
          continue;
        }

        // If a spawn delay is set, the effect doesn't become active immediately
        if (state->spawnDelay > 0)
        {
          state->spawnDelay--;
          continue;
        }

        // Special case for ACT_EXPLOSION_FX_1: Play an explosion sound effect
        // on the first frame.
        if (state->id == ACT_EXPLOSION_FX_1 && state->active == 1)
        {
          if (RandomNumber(ctx) & 1)
          {
            PlaySound(ctx, SND_EXPLOSION);
          }
          else
          {
            PlaySound(ctx, SND_ALTERNATE_EXPLOSION);
          }
        }

        DrawActor(
          ctx, state->id, state->active - 1, state->x, state->y, DS_NORMAL);

        // Special case for ACT_FLAME_FX: Burn away tiles which have the
        // 'flammable' attribute
        if (state->id == ACT_FLAME_FX && state->active == 2)
        {
          // List of pairs of (x, y)
          static sbyte TILE_BURN_OFFSETS[] = {
            0, 0, 0, -1, 0, -2, 1, -2, 2, -2, 2, -1, 2, 0, 1, 0};

          for (j = 0; j < 16; j += 2)
          {
            if (HAS_TILE_ATTRIBUTE(
                  Map_GetTile(
                    ctx,
                    state->x + TILE_BURN_OFFSETS[j],
                    state->y + TILE_BURN_OFFSETS[j + 1]),
                  TA_FLAMMABLE))
            {
              Map_SetTile(
                ctx,
                0,
                state->x + TILE_BURN_OFFSETS[j],
                state->y + TILE_BURN_OFFSETS[j + 1]);
              SpawnEffect(
                ctx,
                ACT_FLAME_FX,
                state->x + TILE_BURN_OFFSETS[j] - 1,
                state->y + TILE_BURN_OFFSETS[j + 1] + 1,
                EM_NONE,
                (int16_t)RandomNumber(ctx) & 3);
            }
          }
        }

        // This keeps track of the effect's life time, and also advances to
        // the next animation frame.
        state->active++;

        if (state->type == EM_RISE_UP)
        {
          state->y--;
        }

        if (state->active == state->framesToLive)
        {
          state->active = 0; // Delete the effect
          continue;
        }
      }
      else if (state->type < 9) // one of EM_FLY_XX or EM_BLOW_IN_WIND
      {
        // Delete effect if it has disappeared from screen *and* has already
        // been alive for 9 frames (movementStep is advanced by 2 every frame)
        if (
          state->movementStep > 17 &&
          !IsSpriteOnScreen(
            ctx, state->id, state->active - 1, state->x, state->y) &&
          state->movementStep > 12) // [NOTE] Redundant, due to the `> 17` above
        {
          state->active = 0;
          continue;
        }

        // If a spawn delay is set, the effect doesn't become active immediately
        if (state->spawnDelay > 0)
        {
          state->spawnDelay--;
          continue;
        }

        DrawActor(
          ctx, state->id, state->active - 1, state->x, state->y, DS_NORMAL);

        // Keep looping the animation
        state->active++;
        if (state->active == state->framesToLive)
        {
          state->active = 1;
        }

        state->x += EFFECT_MOVEMENT_TABLES[state->type][state->movementStep];
        state->y +=
          EFFECT_MOVEMENT_TABLES[state->type][state->movementStep + 1];

        state->movementStep += 2;

        // -127 denotes the end of the movement sequence, at this point, we keep
        // applying the very last movement offset for all subsequent frames
        if (EFFECT_MOVEMENT_TABLES[state->type][state->movementStep] == -127)
        {
          state->movementStep -= 2;
        }
      }

      // Handle effects that damage the player. This doesn't apply to score
      // numbers and burn effect spawners.
      if (
        EffectIsDamaging(state->id) &&
        AreSpritesTouching(
          ctx,
          state->id,
          state->active - 1,
          state->x,
          state->y,
          ctx->plActorId,
          ctx->plAnimationFrame,
          ctx->plPosX,
          ctx->plPosY))
      {
        DamagePlayer(ctx);
      }
    }
  }
}


/** Spawn a player shot into the game world, if possible */
void pascal
  SpawnPlayerShot(Context* ctx, word id, word x, word y, word direction)
{
  register word i;
  register word offset;
  PlayerShot* state;
  word muzzleX = x;
  word muzzleY = y;
  word numFrames;

  switch (id)
  {
    case ACT_DUKE_FLAME_SHOT_UP:
    case ACT_DUKE_FLAME_SHOT_DOWN:
    case ACT_DUKE_FLAME_SHOT_LEFT:
    case ACT_DUKE_FLAME_SHOT_RIGHT:
      PlaySound(ctx, SND_FLAMETHROWER_SHOT);
      break;

    case ACT_DUKE_LASER_SHOT_HORIZONTAL:
    case ACT_DUKE_LASER_SHOT_VERTICAL:
    case ACT_DUKES_SHIP_LASER_SHOT:
      if (id == ACT_DUKES_SHIP_LASER_SHOT)
      {
        muzzleY--;
      }

      PlaySound(ctx, SND_DUKE_LASER_SHOT);
      break;

    default:
      PlaySound(ctx, SND_NORMAL_SHOT);
      break;
  }

  // Adjust position for flame thrower shots to account for the size of the
  // sprite.
  switch (id)
  {
    case ACT_DUKE_FLAME_SHOT_UP:
    case ACT_DUKE_FLAME_SHOT_DOWN:
    case ACT_DUKE_FLAME_SHOT_LEFT:
    case ACT_DUKE_FLAME_SHOT_RIGHT:
      if (direction == SD_UP || direction == SD_DOWN)
      {
        x--;
      }
      else
      {
        y++;
      }
  }

  // Adjust spawn position based on the firing direction. The position always
  // refers to the bottom left of a sprite. The coordinates that are passed
  // into this function are set to match the location of Duke's weapon, which
  // means they are correct when firing to the right or up. But when firing
  // left or down, the position needs to be adjusted by the length of the shot
  // sprite to make it so that the right edge of the shot sprite is next to
  // the left edge of Duke's weapon when firing left, and similarly when
  // firing down.
  if (id == ACT_REGULAR_SHOT_VERTICAL && direction == SD_DOWN)
  {
    y++;
  }
  else if (id == ACT_DUKE_LASER_SHOT_VERTICAL && direction == SD_DOWN)
  {
    y += 3;
  }
  else if (id == ACT_DUKE_LASER_SHOT_HORIZONTAL && direction == SD_LEFT)
  {
    x -= 2;
  }

  // [BUG] Adjustments for rocket shots (left and down) and regular shots
  // (left) are missing. This makes the spawn position of these shots
  // inconsistent between firing left or right - they spawn too far right
  // when firing to the left. Rockets that are fired downwards also spawn
  // too far up.

  offset = ctx->gfxActorInfoData[id];
  numFrames = AINFO_NUM_FRAMES(offset);

  for (i = 0; i < MAX_NUM_PLAYER_SHOTS; i++)
  {
    if (ctx->gmPlayerShotStates[i].active == 0)
    {
      state = ctx->gmPlayerShotStates + i;

      state->active = 1;
      state->id = id;
      state->numFrames = numFrames + 1;
      state->x = x;
      state->y = y;
      state->direction = direction;

      if (state->active < 28) // [NOTE] Always true
      {
        SpawnEffect(
          ctx,
          direction + ACT_MUZZLE_FLASH_UP - SD_UP,
          muzzleX,
          muzzleY,
          EM_NONE,
          0);
      }

      break;
    }
  }
}


/** Update and draw all currently active player shots */
void UpdateAndDrawPlayerShots(Context* ctx)
{
  // See GET_FIELD below. This is a list of memory offsets into the PlayerShot
  // struct, for referencing the y and x fields.
  static const byte OFFSET_TO_POS_FIELD[] = {4, 4, 3, 3};

  // Lookup tables for shot movement. The entries are ordered by shot direction:
  // up, down, left, right. The value is added to the shot's appropriate data
  // field (x or y) in order to move the shot.
  static const sbyte SLOW_SHOT_MOVEMENT[] = {-2, 2, -2, 2};
  static const sbyte FAST_SHOT_MOVEMENT[] = {-5, 5, -5, 5};
  static const sbyte MEDIUM_SHOT_MOVEMENT[] = {-3, 3, -3, 3};

  // This is a list of pairs of (x, y)
  static const sbyte ROCKET_SMOKE_SPAWN_OFFSET[] = {0, 0, 0, -2, 2, 0, 0, 0};

  PlayerShot* state;
  word i;


  // [UNSAFE] This relies on the exact memory layout of the PlayerShot struct.
#define GET_FIELD(dir) *(((word*)state) + OFFSET_TO_POS_FIELD[dir - SD_UP])


  for (i = 0; i < MAX_NUM_PLAYER_SHOTS; i++)
  {
    // Skip deleted shots
    if (ctx->gmPlayerShotStates[i].active == 0)
    {
      continue;
    }

    state = ctx->gmPlayerShotStates + i;

    // TestShotCollision() in game3.c sets the high bit to mark shots that have
    // hit an enemy. These shots are still drawn for one more frame, and then
    // deleted.
    if (state->active & 0x8000)
    {
      // Unset the marker bit, since we need the active field in order to
      // determine the right animation frame.
      state->active &= 0x7FFF;
      DrawActor(
        ctx, state->id, state->active - 1, state->x, state->y, DS_NORMAL);

      state->active = 0; // delete
    }
    else
    {
      // Remove shots that have left the playing field (aka screen)
      if (!IsSpriteOnScreen(
            ctx, state->id, state->active - 1, state->x, state->y))
      {
        state->active = 0; // delete
        continue;
      }

      DrawActor(
        ctx, state->id, state->active - 1, state->x, state->y, DS_NORMAL);

      // Move the shot, according to its type
      switch (state->id)
      {
        case ACT_REGULAR_SHOT_HORIZONTAL:
        case ACT_REGULAR_SHOT_VERTICAL:
          if (CheckWorldCollision(
                ctx,
                MD_PROJECTILE,
                state->id,
                state->active - 1,
                state->x,
                state->y))
          {
            // Spawn a flame at the impact location. This makes it possible to
            // burn flammable tiles with the regular weapon
            // (see UpdateAndDrawEffects()).
            SpawnEffect(
              ctx,
              ACT_FLAME_FX,
              state->x - (state->id == ACT_REGULAR_SHOT_VERTICAL ? 1 : 0),
              state->y + 1,
              EM_RISE_UP,
              0);
            state->active = 0; // delete
          }
          else
          {
            GET_FIELD(state->direction) +=
              SLOW_SHOT_MOVEMENT[state->direction - SD_UP];

            // Animation
            //
            // [NOTE] Unnecessary, since the sprite has only one frame
            state->active++;
            if (state->active == state->numFrames)
            {
              state->active = 1;
            }
          }
          break;

        case ACT_DUKE_LASER_SHOT_HORIZONTAL:
        case ACT_DUKE_LASER_SHOT_VERTICAL:
          // The laser flies through walls, so no collision checking
          GET_FIELD(state->direction) +=
            FAST_SHOT_MOVEMENT[state->direction - SD_UP];
          break;

        case ACT_REACTOR_FIRE_L:
        case ACT_REACTOR_FIRE_R:
        case ACT_DUKES_SHIP_LASER_SHOT:
          // These fly through walls, so no collision checking

          // Animation
          state->active++;
          if (state->active == state->numFrames)
          {
            state->active = 1;
          }

          GET_FIELD(state->direction) +=
            MEDIUM_SHOT_MOVEMENT[state->direction - SD_UP];
          break;


        case ACT_DUKE_FLAME_SHOT_UP:
        case ACT_DUKE_FLAME_SHOT_DOWN:
        case ACT_DUKE_FLAME_SHOT_LEFT:
        case ACT_DUKE_FLAME_SHOT_RIGHT:
          // Somewhat amusingly, the flame thrower *can't* burn away flammable
          // tiles, even though it literally shoots fire. This is because the
          // tile burning is triggered by effects using the ACT_FLAME_FX sprite,
          // not by the player's shots.
          // See UpdateAndDrawEffects()

          // The flame thrower flies through walls, so no collision checking
          GET_FIELD(state->direction) +=
            FAST_SHOT_MOVEMENT[state->direction - SD_UP];
          break;

        case ACT_DUKE_ROCKET_UP:
        case ACT_DUKE_ROCKET_DOWN:
        case ACT_DUKE_ROCKET_LEFT:
        case ACT_DUKE_ROCKET_RIGHT:
          {
            sbyte smokeSpawnX =
              ROCKET_SMOKE_SPAWN_OFFSET[(state->id - SD_UP) * 2];
            sbyte smokeSpawnY =
              ROCKET_SMOKE_SPAWN_OFFSET[(state->id - SD_UP) * 2 + 1];

            if (CheckWorldCollision(
                  ctx,
                  MD_PROJECTILE,
                  state->id,
                  state->active - 1,
                  state->x,
                  state->y))
            {
              // Spawn an explosion effect near the location of impact
              if (state->id < ACT_DUKE_ROCKET_LEFT)
              {
                SpawnEffect(
                  ctx,
                  ACT_EXPLOSION_FX_2,
                  state->x - 2,
                  state->y + 1,
                  EM_NONE,
                  0);
              }
              else
              {
                SpawnEffect(
                  ctx,
                  ACT_EXPLOSION_FX_2,
                  state->x - 1,
                  state->y + 2,
                  EM_NONE,
                  0);
              }

              PlaySound(ctx, SND_EXPLOSION);

              // Spawn flames at the impact location. This makes it possible to
              // burn flammable tiles with the rocket launcher
              // (see UpdateAndDrawEffects()).
              SpawnBurnEffect(ctx, ACT_FLAME_FX, state->id, state->x, state->y);
              state->active = 0; // delete
            }
            else
            {
              // Spawn smoke puffs to mark the rocket's trail
              SpawnEffect(
                ctx,
                ACT_SMOKE_PUFF_FX,
                state->x + smokeSpawnX,
                state->y + smokeSpawnY,
                EM_NONE,
                0);

              GET_FIELD(state->direction) +=
                SLOW_SHOT_MOVEMENT[state->direction - SD_UP];
            }
          }
      }
    }
  }

#undef GET_FIELD
}
