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

Particle effects system

This file implements the game's particle effects. It's a fairly simple system:
Up to 5 groups of 64 particles can exist at a time. Each particle is
represented by a single pixel. Particles follow a pre-calculated path with
some random variation to add variety. Color and time to live are determined
at the group level, so all particles within a group have the same color and
live for the same number of frames.

*******************************************************************************/


/** Initialize the particle system */
void pascal InitParticleSystem(void)
{
  int i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    // Each particle occupies 3 words -> 6 bytes
    psParticleData[i] = MM_PushChunk(PARTICLES_PER_GROUP * 6, CT_COMMON);
  }
}


/** Initialize particle group state with randomized positions & velocities */
static void pascal FillParticleGroup(int index, int direction)
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
        *(psParticleData[index] + a + b*24) =
          direction * (RandomNumber() % 20 + 1);
      }
      else
      {
        // Otherwise, assign a random x velocity between -9 and 10
        *(psParticleData[index] + a + b*24) = 10 - (RandomNumber() % 20);
      }

      // Randomly advance the initial index into the y update lookup table,
      // to make the particle start out flying up or down.
      // See UpdateAndDrawParticles().
      *(psParticleData[index] + a + b*24 + 1) = RandomNumber() & 15; // % 16

      // Initial y offset always starts out at 0
      *(psParticleData[index] + a + b*24 + 2) = 0;
    }
  }
}


/** Erase all currently active particles */
void pascal ClearParticles(void)
{
  register int i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    psParticleGroups[i].timeAlive = 0;
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
void pascal SpawnParticles(word x, word y, sbyte direction, byte color)
{
  register int i;

  for (i = 0; i < NUM_PARTICLE_GROUPS; i++)
  {
    if (psParticleGroups[i].timeAlive == 0)
    {
      ParticleGroup* group = psParticleGroups + i;

      group->timeAlive = 1;
      group->x = x;
      group->y = y;
      group->color = color;

      FillParticleGroup(i, direction);
      break;
    }
  }
}


/** Returns true if the given pixel position is within the viewport area */
static bool pascal IsPointVisible(int x, int y)
{
  if (x >= 8 && x < 264 && y >= 8 && y < 160)
  {
    return true;
  }

  return false;
}


/** Update and draw all currently active particles */
void pascal UpdateAndDrawParticles(void)
{
  // This describes the vertical movement arc for particles: Fly up quickly,
  // slow down near the top of the arc, briefly stop, then fall down slowly
  // and then quicker until max speed is reached.
  //
  // [PERF] Missing `static` causes a copy operation here
  const sbyte MOVEMENT_TABLE[] = {
    -8, -8, -8, -8, -4, -4, -4, -2, -1, 0, 0, 1, 2, 4, 4, 4, 8, 8, 8, 8, 8,
     8,  8,  8,  8,  8,  8,  8,  8,  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
  };

  ParticleGroup* group;
  word i;
  word groupIndex;
  word far* data;
  word x;
  word y;

  EGA_SET_DEFAULT_MODE();

  // [BUG] Setting the EGA map mask to write to all planes simultaneously is
  // missing here. This causes the first SetPixel() to potentially output an
  // incorrect color, depending on the combination of existing color in the
  // framebuffer and particle color. Subsequent calls after the first one are
  // fine, since SetPixel sets the map mask to the correct value after drawing.

  for (groupIndex = 0; groupIndex < NUM_PARTICLE_GROUPS; groupIndex++)
  {
    if (psParticleGroups[groupIndex].timeAlive)
    {
      group = &psParticleGroups[groupIndex];
      data = psParticleData[groupIndex];

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
        x = T2PX(group->x - gmCameraPosX) + *(data + i) * group->timeAlive + 8;

        // Update y position based on lookup table
        *(data + i + 2) += MOVEMENT_TABLE[*(data + i + 1)];
        y = T2PX(group->y - gmCameraPosY) + *(data + i + 2);

        // Advance y update table index
        //
        // [BUG] It's actually possible for this to increment the index past
        // the end of the MOVEMENT_TABLE array, causing some out-of-bounds
        // reads.  tableIndex is initialized to a random number between 0 and
        // 15 (see InitParticleSystem()). Particle groups live for 28 frames.
        // So if a particle has an initial tableIndex of 15, it will reach
        // index 42 on the penultimate frame, and that index will be used to do
        // a table lookup on the 28th frame. MOVEMENT_TABLE only has 41 entries,
        // though. Indices 41 and 42 are thus reading whatever memory follows
        // after MOVEMENT_TABLE. This happens to be the `sysTimerFrequency`
        // variable (see music.c), which is initialized to 280. In little-endian
        // representation, that value corresponds to the number 24 followed by
        // a 1. So any particles which happen to have an initial tableIndex of
        // 14 or 15 will make use of those values.
        *(data + i + 1) += 1;

        // Draw particle if within viewport
        if (IsPointVisible(x, y))
        {
          SetPixel(x, y, group->color);
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
