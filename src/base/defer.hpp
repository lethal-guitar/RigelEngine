/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include <functional>
#include <utility>


namespace rigel::base
{

namespace detail
{

class CallOnDestruction
{
public:
  template <typename Callback>
  explicit CallOnDestruction(Callback&& callback)
    : mCallback(std::forward<Callback>(callback))
  {
  }

  ~CallOnDestruction() { mCallback(); }

  CallOnDestruction(CallOnDestruction&& other) noexcept
    : mCallback(std::exchange(other.mCallback, []() {}))
  {
  }

  CallOnDestruction& operator=(CallOnDestruction&&) = delete;
  CallOnDestruction(const CallOnDestruction&) = delete;
  CallOnDestruction& operator=(const CallOnDestruction&) = delete;

private:
  std::function<void()> mCallback;
};

} // namespace detail


template <typename Callback>
[[nodiscard]] auto defer(Callback&& callback)
{
  return detail::CallOnDestruction{std::forward<Callback>(callback)};
}

} // namespace rigel::base
