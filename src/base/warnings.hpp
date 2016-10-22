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

/* This file defines macros which can be used to temporarily disable compiler
 * warnings. This is mainly useful when including 3rd party headers.
 */

#if defined(_MSC_VER)

  #define RIGEL_DISABLE_GLOBAL_CTORS_WARNING __pragma(warning(push))

  #define RIGEL_DISABLE_WARNINGS __pragma(warning(push, 0))

  #define RIGEL_RESTORE_WARNINGS __pragma(warning(pop))

#elif defined(__clang__)

  #define RIGEL_DISABLE_GLOBAL_CTORS_WARNING \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
    /**/

  #define RIGEL_DISABLE_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Weverything\"") \
    /**/

  #define RIGEL_RESTORE_WARNINGS _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

  #define RIGEL_DISABLE_GLOBAL_CTORS_WARNING _Pragma("GCC diagnostic push")

  #define RIGEL_DISABLE_WARNINGS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
    /**/

  #define RIGEL_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")

#else

  #error "Unrecognized compiler!"

#endif
