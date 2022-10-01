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

Memory manager

Unlike Cosmo, Duke Nukem II features a custom memory manager. The game performs
only a single OS allocation via farmalloc during startup, and then manages all
subsequent memory allocations via its own manager. This has quite a few
advantages:

* The exact memory requirements are known and it's easy to test for sufficient
  memory during startup
* It's easier to output debug information on currently active allocatios, which
  was likely a big help for keeping an eye on memory usage at runtime and
  tracking down memory leaks
* Since the manager doesn't have to be general purpose, it can be made much more
  efficient compared to the standard library

In fact, the design of the manager makes it extremely fast, with O(1)
allocations and deallocations: It's a stack-based allocator, where allocating a
block simply requires a few additions and memory stores. The downside of this
simplicity is that blocks _must_ be deallocated in reverse order of allocation,
it's not possible to deallocate a block that's not the most recently allocated
one. This means that the ordering of allocations and deallocations is extremely
important and has to be carefully managed throughout the entire application.

One other limitation that's specific to this memory manager is that there's an
upper bound not only on total memory, but also on the number of allocations
(called chunks by the game). This is because the bookkeeping meta data is stored
in fixed-size arrays. This could be improved by storing the bookkeeping data
interleaved with the memory itself, instead of in a separate array. When
allocating, we'd take a few extra bytes on top of what the client requested.
We'd then store the size and chunk type in those extra bytes. When deallocating,
we go backwards from the top of memory to find the chunk type and size. This
design would eliminate the limitation on the number of chunks, without adding
any extra overhead - the memory used by the arrays would just shift into the
managed memory pool.

Chunks are categorized into types. This makes it possible to deallocate multiple
blocks of the same type, e.g. sprite data, in one go.  Deallocation is in fact
done just by specifying the chunk type, not the allocated pointer.

*******************************************************************************/


/** Initialize the memory manager
 *
 * This allocates 380 kB of memory from DOS, and sets up the memory manager.
 * Doesn't do any error checking, and always returns false.
 */
bool MM_Init(void)
{
  int i;

  // Reset the per-chunk bookkeeping data.
  //
  // [NOTE] I don't think this is necessary, none of this data is used as long
  // as mmChunksUsed is 0, and as chunks are added, only those with valid data
  // are used. It doesn't hurt to set these to a known state, of course. It
  // could've been done more efficiently using memset, though.
  for (i = 0; i < MM_MAX_NUM_CHUNKS; ++i)
  {
    mmChunkSizes[i] = 0;
    mmChunkTypes[i] = CT_TEMPORARY;
  }

  // Allocate memory from OS (via Borland standard library) and initialize
  // everything else.
  mmMemTotal = MM_TOTAL_SIZE;
  mmRawMem = farmalloc(MM_TOTAL_SIZE);
  mmChunksUsed = 0;
  mmMemUsed = 0;

  return false;
}


/** Returns amount of remaining free memory */
dword MM_MemAvailable(void)
{
  return mmMemTotal - mmMemUsed;
}


#define CURRENT_MEM_TOP_PTR() (void far*)((byte huge*)mmRawMem + mmMemUsed)


/** Allocate a chunk of given size and type
 *
 * Allocates a block of memory with the requested size and type, and returns it.
 * Terminates the program if the request can't be fulfilled (either due to not
 * enough memory being left, or the maximum number of chunks already being
 * allocated).
 */
void far* MM_PushChunk(word size, ChunkType type)
{
  void far* mem;

  if (mmMemUsed + size > mmMemTotal)
  {
    Quit("No Memory");
  }

  if (mmChunksUsed >= MM_MAX_NUM_CHUNKS)
  {
    Quit("No Chunks");
  }
  else
  {
    // Make a note of the newly allocated chunk's properties
    mmChunkSizes[mmChunksUsed] = size;
    mmChunkTypes[mmChunksUsed] = type;

    // Use current top of memory buffer to satisfy the request
    mem = CURRENT_MEM_TOP_PTR();

    // Update how much memory/chunks remain available
    mmMemUsed += size;
    mmChunksUsed++;
  }

  return mem;
}


/** Nulls out sprite data pointers matching the given address */
static void pascal UpdateSpriteDataList(void far* address)
{
  word i;

  for (i = 0; i < MM_MAX_NUM_CHUNKS; i++)
  {
    if (gfxLoadedSprites[i] == address)
    {
      gfxLoadedSprites[i] = NULL;

      // [PERF] The search could be terminated here, since only one sprite
      // can be loaded at one address at a time. But the loop keeps running
      // until all sprite data pointers have been checked.
    }
  }
}


/** Frees last allocated chunk
 *
 * Frees the most recently allocated chunk _iff_ the given type matches. No-op
 * otherwise.
 *
 * If the chunk type is CT_INGAME_MUSIC, this also calls StopMusic().
 * No special handling for CT_SPRITE chunks, unlike MM_PopChunks.
 *
 * Example:
 *
 *   byte far* data = MM_PushChunk(5, CT_TEMPORARY);
 *
 *   // work with data
 *
 *   MM_PopChunk(CT_TEMPORARY);
 */
void pascal MM_PopChunk(ChunkType type)
{
  if (mmChunksUsed != 0 && mmChunkTypes[mmChunksUsed-1] == type)
  {
    mmChunksUsed--;
    mmMemUsed -= mmChunkSizes[mmChunksUsed];

    if (type == CT_INGAME_MUSIC)
    {
      StopMusic();
    }
  }
}


/** Frees multiple chunks
 *
 * Frees all most recently allocated chunks _iff_ the given type matches.  Stops
 * as soon as a chunk of different type is encountered.  This means that if
 * several chunks of type A have been allocated followed by a chunk of type B
 * and then more chunks of type A, only the chunks following the B chunk will be
 * freed.
 *
 * If the chunk type is CT_SPRITE, the corresponding entries in the loaded
 * sprite table will be reset to NULL. No special handling for CT_INGAME_MUSIC,
 * unlike MM_PopChunk.
 *
 * Example:
 *
 *   MM_PushChunk(5, CT_TEMPORARY);    // not freed by call below
 *   MM_PushChunk(5, CT_INGAME_MUSIC); // not freed
 *   MM_PushChunk(5, CT_TEMPORARY);    // freed
 *   MM_PushChunk(5, CT_TEMPORARY);    // freed
 *
 *   MM_PopChunks(CT_TEMPORARY);
 */
void pascal MM_PopChunks(ChunkType type)
{
  while (mmChunksUsed != 0 && mmChunkTypes[mmChunksUsed-1] == type)
  {
    mmChunksUsed--;
    mmMemUsed -= mmChunkSizes[mmChunksUsed];

    if (type == CT_SPRITE)
    {
      // [PERF] UpdateSpriteDataList internally does a linear search for the
      // address, making this an O(N^2) algorithm.  This isn't so easy to fix
      // though, because the memory manager doesn't know which sprite data index
      // is used for which address. And any extra bookkeeping data would require
      // more memory, which is in short supply. The game doesn't unload any
      // sprites during gameplay, so there's no performance impact on the game
      // itself and this is probably not a bad design overall, since it's so
      // simple.
      UpdateSpriteDataList(CURRENT_MEM_TOP_PTR());
    }
  }
}
