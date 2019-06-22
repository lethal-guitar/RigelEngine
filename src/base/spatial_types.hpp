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

#include "base/math_tools.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_rect.h>
RIGEL_RESTORE_WARNINGS


#include <cstdint>
#include <tuple>


namespace rigel::base {

template<typename ValueT>
struct Rect;

namespace detail {

template<typename ValueT>
SDL_Rect toSdlRect(const Rect<ValueT>& rect)
{
  return {
    int(rect.topLeft.x), int(rect.topLeft.y),
    int(rect.size.width), int(rect.size.height)
  };
}

}

template<typename ValueT>
struct Point {
  Point() = default;
  Point(const Point&) = default;
  Point(Point&&) = default;
  constexpr Point(const ValueT x_, const ValueT y_) noexcept
    : x(x_)
    , y(y_)
  {
  }
  Point& operator=(const Point&) = default;

  bool operator==(const Point<ValueT>& rhs) const {
    return std::tie(x, y) == std::tie(rhs.x, rhs.y);
  }

  bool operator!=(const Point<ValueT>& rhs) const {
    return !(*this == rhs);
  }


  ValueT x = 0;
  ValueT y = 0;
};


template<typename ValueT>
struct Size {
  Size() = default;
  constexpr Size(const ValueT width_, const ValueT height_) noexcept
    : width(width_)
    , height(height_)
  {
  }

  bool operator==(const Size<ValueT>& rhs) const {
    return std::tie(width, height) == std::tie(rhs.width, rhs.height);
  }

  bool operator!=(const Size<ValueT>& rhs) const {
    return !(*this == rhs);
  }

  ValueT width = 0;
  ValueT height = 0;
};


template<typename ValueT>
struct Rect {
  Point<ValueT> topLeft;
  Size<ValueT> size;

  Point<ValueT> bottomLeft() const {
    return Point<ValueT>{
      topLeft.x,
      static_cast<ValueT>(topLeft.y + (size.height - 1))
    };
  }

  Point<ValueT> bottomRight() const {
    return bottomLeft() + Point<ValueT>{
      static_cast<ValueT>(size.width - 1),
      0
    };
  }

  ValueT top() const {
    return topLeft.y;
  }

  ValueT bottom() const {
    return bottomLeft().y;
  }

  ValueT left() const {
    return topLeft.x;
  }

  ValueT right() const {
    return bottomRight().x;
  }

  bool intersects(const Rect& other) const {
    const auto r1 = detail::toSdlRect(*this);
    const auto r2 = detail::toSdlRect(other);
    return SDL_HasIntersection(&r1, &r2);
  }

  bool containsPoint(const Point<ValueT>& point) const {
    return
      inRange(point.x, left(), right()) && inRange(point.y, top(), bottom());
  }
};


template<typename ValueT>
Rect<ValueT> makeRect(Point<ValueT> topLeft, Point<ValueT> bottomRight) {
  const auto sizeAsPoint = bottomRight - topLeft;
  return Rect<ValueT>{topLeft, Size<ValueT>{sizeAsPoint.x, sizeAsPoint.y}};
}


template<typename ValueT>
Point<ValueT> operator+(
  const Point<ValueT>& lhs,
  const Point<ValueT>& rhs
) {
  return Point<ValueT>(lhs.x + rhs.x, lhs.y + rhs.y);
}


template<typename ValueT>
Point<ValueT> operator-(
  const Point<ValueT>& lhs,
  const Point<ValueT>& rhs
) {
  return Point<ValueT>(lhs.x - rhs.x, lhs.y - rhs.y);
}


template<typename ValueT, typename ScalarT>
auto operator*(
  const Point<ValueT>& point,
  const ScalarT scalar
) {
  return Point<decltype(point.x * scalar)>{
    point.x * scalar,
    point.y * scalar
  };
}


template<typename ValueT>
Point<ValueT>& operator+=(Point<ValueT>& lhs, const Point<ValueT>& rhs) {
  auto newPoint = lhs + rhs;
  std::swap(lhs, newPoint);
  return lhs;
}


template<typename ValueT>
Point<ValueT>& operator-=(Point<ValueT>& lhs, const Point<ValueT>& rhs) {
  auto newPoint = lhs - rhs;
  std::swap(lhs, newPoint);
  return lhs;
}


template<typename ValueT>
Rect<ValueT> operator+(
  const Rect<ValueT>& rect,
  const Point<ValueT>& translation
) {
  return Rect<ValueT>{
    rect.topLeft + Point<ValueT>{
        translation.x,
        translation.y},
    rect.size
  };
}


template<typename ValueT>
bool operator==(const Rect<ValueT>& lhs, const Rect<ValueT>& rhs) {
  return std::tie(lhs.topLeft, lhs.size) == std::tie(rhs.topLeft, rhs.size);
}


template<typename ValueT>
bool operator!=(const Rect<ValueT>& lhs, const Rect<ValueT>& rhs) {
  return !(lhs == rhs);
}


using Vector = Point<int>;
using Extents = Size<int>;

}
