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

#include "game.h"


/*******************************************************************************

Particle effects system

This file implements the game's particle effects. It's a fairly simple system:
Up to 5 groups of 64 particles can exist at a time. Each particle is
represented by a single pixel. Particles follow a pre-calculated path with
some random variation to add variety. Color and time to live are determined
at the group level, so all particles within a group have the same color and
live for the same number of frames.

*******************************************************************************/


/** Initialize the particle system */
void InitParticleSystem(Context* ctx)
{
  int16_t i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    // Each particle occupies 3 words -> 6 bytes
    ctx->psParticleData[i] =
      MM_PushChunk(ctx, PARTICLES_PER_GROUP * 6, CT_COMMON);
  }
}


/** Initialize particle group state with randomized positions & velocities */
static void FillParticleGroup(Context* ctx, int16_t index, int16_t direction)
{
  sbyte a;
  sbyte b;

  // [NOTE] I'm not sure why this is a nested loop. The end result is the same
  // as a single loop going from 0 to 8 * 8 * 3.
  for (b = 0; b < 8; b++)
  {
    // Add 3 on each iteration to skip one particle state entry (3 words).
    // Per-particle state is stored as a plain array of word values, not
    // structs. The data layout is:
    //
    // 0: word xVelocity;
    // 1: word tableIndex;
    // 2: word yOffset;
    for (a = 0; a < 8 * 3; a = a + 3)
    {
      if (direction)
      {
        // If a direction is specified, scale it randomly by a value between
        // 1 and 20 to generate a velocity.
        *(ctx->psParticleData[index] + a + b * 24) =
          (word)(direction * (RandomNumber(ctx) % 20 + 1));
      }
      else
      {
        // Otherwise, assign a random x velocity between -9 and 10
        *(ctx->psParticleData[index] + a + b * 24) =
          10 - (RandomNumber(ctx) % 20);
      }

      // Randomly advance the initial index into the y update lookup table,
      // to make the particle start out flying up or down.
      // See UpdateAndDrawParticles().
      *(ctx->psParticleData[index] + a + b * 24 + 1) =
        RandomNumber(ctx) & 15; // % 16

      // Initial y offset always starts out at 0
      *(ctx->psParticleData[index] + a + b * 24 + 2) = 0;
    }
  }
}


/** Erase all currently active particles */
void ClearParticles(Context* ctx)
{
  register int16_t i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    ctx->psParticleGroups[i].timeAlive = 0;
  }
}


/** Spawn a new group of particles into the game world
 *
 * Does nothing if all 5 particle groups are already in use.
 *
 * [NOTE] Due to the short period of the random number generator (see
 * coreutil.c), only two successive calls to this function can be made without
 * any other random number generation in-between. The problem is that one call
 * to this function consumes exactly 128 random numbers, and after 256 numbers,
 * the sequence starts repeating. Due to this, the particles resulting from the
 * 3rd call to this function end up in exactly the same positions and with
 * exactly the same velocities as those created by the 1st call, and thus they
 * will overlap the 1st call's particles and effectively render them invisible.
 */
void SpawnParticles(Context* ctx, word x, word y, sbyte direction, byte color)
{
  register int16_t i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    if (ctx->psParticleGroups[i].timeAlive == 0)
    {
      ParticleGroup* group = ctx->psParticleGroups + i;

      group->timeAlive = 1;
      group->x = x;
      group->y = y;
      group->color = color;

      FillParticleGroup(ctx, i, direction);
      break;
    }
  }
}


/** Returns true if the given pixel position is within the viewport area */
static bool IsPointVisible(int16_t x, int16_t y)
{
  if (x >= 8 && x < 264 && y >= 8 && y < 160)
  {
    return true;
  }

  return false;
}


/** Update and draw all currently active particles */
void UpdateAndDrawParticles(Context* ctx)
{
  // This describes the vertical movement arc for particles: Fly up quickly,
  // slow down near the top of the arc, briefly stop, then fall down slowly
  // and then quicker until max speed is reached.
  //
  // clang-format off
  const sbyte MOVEMENT_TABLE[] = {
    -8, -8, -8, -8, -4, -4, -4, -2, -1, 0, 0, 1, 2, 4, 4, 4, 8, 8, 8, 8, 8,
     8,  8,  8,  8,  8,  8,  8,  8,  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0
  };
  // clang-format on

  ParticleGroup* group;
  word i;
  word groupIndex;
  word* data;
  word x;
  word y = 32; // see below

  for (groupIndex = 0; groupIndex < NUM_PARTICLE_GROUPS; groupIndex++)
  {
    if (ctx->psParticleGroups[groupIndex].timeAlive)
    {
      group = &ctx->psParticleGroups[groupIndex];
      data = ctx->psParticleData[groupIndex];

      for (i = 0; i < PARTICLES_PER_GROUP * 3; i += 3)
      {
        // Per-particle state is stored as a plain array of word values, not
        // structs. The data layout is:
        //
        // 0: word xVelocity;
        // 1: word tableIndex;
        // 2: word yOffset;

        // Calculate x position: Simple linear movement according to velocity,
        // i.e. x = xVelocity * timeAlive. For some reason, all x positions
        // are also offset to the right by 1 tile/8 pixels.
        x = (word)(
          T2PX(group->x - ctx->gmCameraPosX) + *(data + i) * group->timeAlive +
          8);

        // Update y position based on lookup table

        // Recreate out-of-bounds read from the original code (reading past the
        // end of MOVEMENT_TABLE, which has been copied to the stack, thus
        // reading the low byte of the `y` variable).
        word tableIndex = *(data + i + 1);
        sbyte adjustment = tableIndex == 42
          ? (sbyte)(y & 0xFF) : MOVEMENT_TABLE[tableIndex];

        *(data + i + 2) += adjustment;
        y = (word)(T2PX(group->y - ctx->gmCameraPosY) + *(data + i + 2));

        // Advance y update table index
        *(data + i + 1) += 1;

        // Draw particle if within viewport
        if (IsPointVisible(x, y))
        {
          SetPixel(ctx, x, y, (byte)group->color);
        }
      }

      ++group->timeAlive;
      if (group->timeAlive == 29)
      {
        group->timeAlive = 0;
      }
    }
  }
}
