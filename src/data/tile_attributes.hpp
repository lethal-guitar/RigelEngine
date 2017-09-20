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

#include <cstddef>
#include <vector>

#include "image.hpp"


namespace rigel { namespace data { namespace map {

using TileIndex = std::uint32_t;


class SolidEdge {
public:
  static SolidEdge top();
  static SolidEdge bottom();
  static SolidEdge left();
  static SolidEdge right();

  friend class CollisionData;

private:
  explicit SolidEdge(const std::uint8_t bitPack)
    : mFlagsBitPack(bitPack)
  {
  }

  std::uint8_t mFlagsBitPack;
};


class CollisionData {
public:
  static CollisionData fullySolid();

  CollisionData() = default;
  CollisionData(const std::initializer_list<CollisionData>& items);
  explicit CollisionData(std::uint8_t flagsBitPack);

  bool isSolidOn(const SolidEdge& edge) const {
    return (mCollisionFlagsBitPack & edge.mFlagsBitPack) != 0;
  }

private:
  std::uint8_t mCollisionFlagsBitPack = 0;
};


class TileAttributes {
public:
  using AttributeArray = std::vector<std::uint16_t>;

  TileAttributes() = default;
  explicit TileAttributes(const AttributeArray& bitpacks);
  explicit TileAttributes(AttributeArray&& bitpacks);

  bool isAnimated(TileIndex tile) const;
  bool isFastAnimation(TileIndex tile) const;
  bool isForeGround(TileIndex tile) const;
  bool isLadder(TileIndex tile) const;

  CollisionData collisionData(TileIndex tile) const;

private:
  std::uint16_t bitPackFor(TileIndex tile) const;

private:
  AttributeArray mAttributeBitPacks;
};

}}}
