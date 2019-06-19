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

#include "base/spatial_types.hpp"


namespace rigel::engine { namespace components {

using WorldPosition = base::Vector;
using BoundingBox = base::Rect<int>;

/** Marks entity as active
 *
 * Most systems should only operate on active entities. Entity activation
 * depends on their ActivationSettings - by default, entities will only be
 * active if their bounding box intersects the active region, i.e. they are
 * visible on screen.
 * */
struct Active {
  bool mIsOnScreen = true;
};

/** Specifies when to activate entity */
struct ActivationSettings {
  enum class Policy {
    // Entity is always active
    Always,

    // Entity is inactive until it appeared on screen once, it remains
    // active from then on
    AlwaysAfterFirstActivation,

    // Entity is only active while on screen. Specifically, its bounding box
    // must intersect the active region.
    WhenOnScreen
  };

  ActivationSettings() = default;
  explicit ActivationSettings(const Policy policy)
    : mPolicy(policy)
  {
  }

  Policy mPolicy = Policy::WhenOnScreen;
  bool mHasBeenActivated = false;
};


enum class Orientation {
  Left,
  Right
};

}


namespace orientation {

inline components::Orientation opposite(
  const components::Orientation orientation
) {
  return orientation == components::Orientation::Left
    ? components::Orientation::Right
    : components::Orientation::Left;
}


inline int toMovement(const components::Orientation orientation) {
  return orientation == components::Orientation::Left ? -1 : 1;
}

}

}
