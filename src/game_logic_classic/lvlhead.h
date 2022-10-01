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

#ifndef LVL_HEAD_H
#define LVL_HEAD_H

#define LVL_TILESET_FILENAME() (levelHeaderData)
#define LVL_BACKDROP_FILENAME() (levelHeaderData + 13)
#define LVL_MUSIC_FILENAME() (levelHeaderData + 26)

#define READ_LVL_HEADER_WORD(offset)        \
  ((*(levelHeaderData + offset + 1) << 8) | \
   *(levelHeaderData + offset))

// Utility macros for reading actor descriptions in the level header
//
// 45 is the offset of the first actor description's first word within the level
// header. Each actor description consists of 3 words (id, x, y).
#define READ_LVL_ACTOR_DESC_ID(index) READ_LVL_HEADER_WORD(45 + index)
#define READ_LVL_ACTOR_DESC_X(index)  READ_LVL_HEADER_WORD(47 + index)
#define READ_LVL_ACTOR_DESC_Y(index)  READ_LVL_HEADER_WORD(49 + index)

#endif
