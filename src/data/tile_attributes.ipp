/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <stdexcept>


namespace rigel::data::map
{

using namespace std;


namespace detail
{

inline bool isBitSet(uint16_t bitPack, uint16_t bitMask)
{
  return (bitPack & bitMask) != 0;
}

} // namespace detail


inline SolidEdge SolidEdge::top()
{
  return SolidEdge{0x01};
}


inline SolidEdge SolidEdge::bottom()
{
  return SolidEdge{0x02};
}


inline SolidEdge SolidEdge::left()
{
  return SolidEdge{0x08};
}


inline SolidEdge SolidEdge::right()
{
  return SolidEdge{0x04};
}


inline SolidEdge SolidEdge::any()
{
  return SolidEdge{0x0F};
}


inline CollisionData CollisionData::fullySolid()
{
  return CollisionData{0xFF};
}


inline CollisionData::CollisionData(
  const std::initializer_list<CollisionData>& items)
{
  for (const auto& item : items)
  {
    mCollisionFlagsBitPack |= item.mCollisionFlagsBitPack;
  }
}


inline CollisionData::CollisionData(const std::uint8_t flagsBitPack)
  : mCollisionFlagsBitPack(flagsBitPack)
{
}


inline TileAttributes::TileAttributes(const std::uint16_t attributesBitPack)
  : mAttributesBitPack(attributesBitPack)
{
}


inline bool TileAttributes::isAnimated() const
{
  return detail::isBitSet(mAttributesBitPack, 0x10);
}


inline bool TileAttributes::isFastAnimation() const
{
  return !detail::isBitSet(mAttributesBitPack, 0x400);
}


inline bool TileAttributes::isForeGround() const
{
  return detail::isBitSet(mAttributesBitPack, 0x20);
}


inline bool TileAttributes::isLadder() const
{
  return detail::isBitSet(mAttributesBitPack, 0x4000);
}


inline bool TileAttributes::isClimbable() const
{
  return detail::isBitSet(mAttributesBitPack, 0x80);
}


inline bool TileAttributes::isConveyorBeltLeft() const
{
  return detail::isBitSet(mAttributesBitPack, 0x100);
}


inline bool TileAttributes::isConveyorBeltRight() const
{
  return detail::isBitSet(mAttributesBitPack, 0x200);
}


inline bool TileAttributes::isFlammable() const
{
  return detail::isBitSet(mAttributesBitPack, 0x40);
}


inline TileAttributeDict::TileAttributeDict(const AttributeArray& bitpacks)
  : mAttributeBitPacks(bitpacks)
{
}


inline TileAttributeDict::TileAttributeDict(AttributeArray&& bitpacks)
  : mAttributeBitPacks(std::move(bitpacks))
{
}


inline uint16_t TileAttributeDict::bitPackFor(const TileIndex tile) const
{
  assert(tile < mAttributeBitPacks.size());
  return mAttributeBitPacks[tile];
}


inline TileAttributes TileAttributeDict::attributes(const TileIndex tile) const
{
  return TileAttributes(bitPackFor(tile));
}


inline CollisionData
  TileAttributeDict::collisionData(const TileIndex tile) const
{
  return CollisionData(static_cast<uint8_t>(bitPackFor(tile) & 0xF));
}

} // namespace rigel::data::map
