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

#include "tile_attributes.hpp"

#include <cassert>
#include <stdexcept>


namespace rigel { namespace data { namespace map {

using namespace std;


namespace {

bool isBitSet(uint16_t bitPack, uint16_t bitMask) {
  return (bitPack & bitMask) != 0;
}

}


SolidEdge SolidEdge::top() {
  return SolidEdge{0x01};
}


SolidEdge SolidEdge::bottom() {
  return SolidEdge{0x02};
}


SolidEdge SolidEdge::left() {
  return SolidEdge{0x08};
}


SolidEdge SolidEdge::right() {
  return SolidEdge{0x04};
}


CollisionData CollisionData::fullySolid() {
  return CollisionData{0xFF};
}


CollisionData::CollisionData(const std::initializer_list<CollisionData>& items)
{
  for (const auto& item : items) {
    mCollisionFlagsBitPack |= item.mCollisionFlagsBitPack;
  }
}


CollisionData::CollisionData(const std::uint8_t flagsBitPack)
  : mCollisionFlagsBitPack(flagsBitPack)
{
}


TileAttributes::TileAttributes(const std::uint16_t attributesBitPack)
  : mAttributesBitPack(attributesBitPack)
{
}


bool TileAttributes::isAnimated() const {
  return isBitSet(mAttributesBitPack, 0x10);
}


bool TileAttributes::isFastAnimation() const {
  return !isBitSet(mAttributesBitPack, 0x400);
}


bool TileAttributes::isForeGround() const {
  return isBitSet(mAttributesBitPack, 0x20);
}


bool TileAttributes::isLadder() const {
  return isBitSet(mAttributesBitPack, 0x4000);
}


bool TileAttributes::isClimbable() const {
  return isBitSet(mAttributesBitPack, 0x80);
}


bool TileAttributes::isConveyorBeltLeft() const {
  return isBitSet(mAttributesBitPack, 0x100);
}


bool TileAttributes::isConveyorBeltRight() const {
  return isBitSet(mAttributesBitPack, 0x200);
}


bool TileAttributes::isFlammable() const {
  return isBitSet(mAttributesBitPack, 0x40);
}


TileAttributeDict::TileAttributeDict(const AttributeArray& bitpacks)
  : mAttributeBitPacks(bitpacks)
{
}


TileAttributeDict::TileAttributeDict(AttributeArray&& bitpacks)
  : mAttributeBitPacks(move(bitpacks))
{
}


uint16_t TileAttributeDict::bitPackFor(const TileIndex tile) const {
  assert(tile < mAttributeBitPacks.size());
  return mAttributeBitPacks[tile];
}


TileAttributes TileAttributeDict::attributes(const TileIndex tile) const {
  return TileAttributes(bitPackFor(tile));
}


CollisionData TileAttributeDict::collisionData(const TileIndex tile) const {
  return CollisionData(static_cast<uint8_t>(bitPackFor(tile) & 0xF));
}

}}}
