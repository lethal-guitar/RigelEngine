/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
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

#include "array_view.hpp"

#include <stdexcept>
#include <string>


namespace rigel::base::detail
{

[[noreturn]] void throwOutOfRange(std::uint32_t index)
{
  // This is an optimization for code size and compile times. To avoid
  // having every client of ArrayView pull in the (fairly expensive)
  // stdexcept and string headers into their translation units, and reduce
  // the amount of code generated for each instantiation of ArrayView::at().
  using namespace std::string_literals;

  throw std::range_error("Index out of range: "s + std::to_string(index));
}

} // namespace rigel::base::detail
