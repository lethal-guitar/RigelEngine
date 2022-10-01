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

#ifndef DEFS_H
#define DEFS_H

#include "types.h"

// Pixels
#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           200

// Tiles
#define SCREEN_WIDTH_TILES       40
#define SCREEN_HEIGHT_TILES      25
#define VIEWPORT_WIDTH           32
#define VIEWPORT_HEIGHT          20

#define TIMER_FREQUENCY         280

#define CLOAK_TIME              700
#define RAPID_FIRE_TIME         700
#define MAX_AMMO                 32
#define MAX_AMMO_FLAMETHROWER    64
#define PLAYER_MAX_HEALTH         9
#define INITIAL_MERCY_FRAMES     20
#define NUM_INVENTORY_SLOTS       6

#define MM_TOTAL_SIZE        390000
#define MM_MAX_NUM_CHUNKS      1150

#define NUM_HIGH_SCORE_ENTRIES   10
#define HIGH_SCORE_NAME_MAX_LEN  15

#define NUM_SAVE_SLOTS            8
#define SAVE_SLOT_NAME_MAX_LEN   18

#define NUM_PARTICLE_GROUPS       5
#define PARTICLES_PER_GROUP      64

#define MAX_NUM_ACTORS          448
#define MAX_NUM_EFFECTS          18
#define MAX_NUM_PLAYER_SHOTS      6
#define MAX_NUM_MOVING_MAP_PARTS 70


typedef enum
{
  CLR_BLACK = 0,
  CLR_DARK_GREY = 1,
  CLR_GREY = 2,
  CLR_LIGHT_GREY = 3,
  CLR_DARK_RED = 4,
  CLR_RED = 5,
  CLR_ORANGE = 6,
  CLR_YELLOW = 7,
  CLR_DARK_GREEN = 8,
  CLR_DARK_BLUE = 9,
  CLR_BLUE = 10,
  CLR_LIGHT_BLUE = 11,
  CLR_GREEN = 12,
  CLR_LIGHT_GREEN = 13,
  CLR_BROWN = 14,
  CLR_WHITE = 15
} PaletteColor;


#ifndef SHAREWARE
#define REGISTERED_VERSION
#endif


#endif
