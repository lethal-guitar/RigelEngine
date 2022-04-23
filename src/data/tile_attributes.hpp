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

#pragma once

#include "base/image.hpp"

#include <cstddef>
#include <vector>


namespace rigel::data::map
{

using TileIndex = std::uint32_t;


class SolidEdge
{
public:
  static SolidEdge top();
  static SolidEdge bottom();
  static SolidEdge left();
  static SolidEdge right();
  static SolidEdge any();

  friend class CollisionData;

private:
  explicit SolidEdge(const std::uint8_t bitPack)
    : mFlagsBitPack(bitPack)
  {
  }

  std::uint8_t mFlagsBitPack;
};


class CollisionData
{
public:
  static CollisionData fullySolid();

  CollisionData() = default;
  CollisionData(const std::initializer_list<CollisionData>& items);
  explicit CollisionData(std::uint8_t flagsBitPack);

  bool isSolidOn(const SolidEdge& edge) const
  {
    return (mCollisionFlagsBitPack & edge.mFlagsBitPack) != 0;
  }

private:
  std::uint8_t mCollisionFlagsBitPack = 0;
};


class TileAttributes
{
public:
  TileAttributes() = default;
  explicit TileAttributes(std::uint16_t attributesBitPack);

  bool isAnimated() const;
  bool isFastAnimation() const;
  bool isForeGround() const;
  bool isLadder() const;
  bool isClimbable() const;
  bool isConveyorBeltLeft() const;
  bool isConveyorBeltRight() const;
  bool isFlammable() const;

private:
  std::uint16_t mAttributesBitPack = 0;
};


class TileAttributeDict
{
public:
  using AttributeArray = std::vector<std::uint16_t>;

  TileAttributeDict() = default;
  explicit TileAttributeDict(const AttributeArray& bitpacks);
  explicit TileAttributeDict(AttributeArray&& bitpacks);

  TileAttributes attributes(TileIndex tile) const;
  CollisionData collisionData(TileIndex tile) const;

private:
  std::uint16_t bitPackFor(TileIndex tile) const;

private:
  AttributeArray mAttributeBitPacks;
};

} // namespace rigel::data::map

#include "data/tile_attributes.ipp"
