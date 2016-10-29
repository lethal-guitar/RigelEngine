# FindSDL2_mixer
# -------------
#
# Locate SDL_mixer library for SDL2

# This file is a modified version of the file FindSDL_mixer.cmake bundled with
# CMake.
#
# This module defines:
#
# ::
#
#   SDL2_MIXER_LIBRARIES, the name of the library to link against
#   SDL2_MIXER_INCLUDE_DIRS, where to find the headers
#   SDL2_MIXER_FOUND, if false, do not try to link against
#   SDL2_MIXER_VERSION_STRING - human-readable string containing the
#                              version of SDL_mixer
#
#
# $SDL2DIR is an environment variable that would correspond to the
# ./configure --prefix=$SDLDIR used in building SDL.
#
# Created by Eric Wing.  This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


find_package(PkgConfig QUIET)
pkg_check_modules(PKG_CONF_SDL2_MIXER QUIET SDL2_mixer>=2.0.0)

find_path(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h
  HINTS
    ENV SDL2MIXERDIR
    ENV SDL2DIR
    ${PKG_CONF_SDL2_MIXER_INCLUDEDIR}
    ${PKG_CONF_SDL2_MIXER_INCLUDE_DIRS}
  PATH_SUFFIXES SDL2
                # path suffixes to search inside ENV{SDL2DIR}
                include/SDL include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(SDL2_MIXER_LIBRARY
  NAMES SDL2_mixer
  HINTS
    ENV SDL2MIXERDIR
    ENV SDL2DIR
    ${PKG_CONF_SDL2_MIXER_LIBDIR}
    ${PKG_CONF_SDL2_MIXER_LIBRARY_DIRS}
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

if(SDL2_MIXER_INCLUDE_DIR AND EXISTS "${SDL2_MIXER_INCLUDE_DIR}/SDL_mixer.h")
  file(STRINGS "${SDL2_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_MIXER_INCLUDE_DIR}/SDL_mixer.h" SDL_MIXER_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_MAJOR "${SDL_MIXER_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_MINOR "${SDL_MIXER_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL_MIXER_VERSION_PATCH "${SDL_MIXER_VERSION_PATCH_LINE}")
  set(SDL2_MIXER_VERSION_STRING ${SDL_MIXER_VERSION_MAJOR}.${SDL_MIXER_VERSION_MINOR}.${SDL_MIXER_VERSION_PATCH})
  unset(SDL_MIXER_VERSION_MAJOR_LINE)
  unset(SDL_MIXER_VERSION_MINOR_LINE)
  unset(SDL_MIXER_VERSION_PATCH_LINE)
  unset(SDL_MIXER_VERSION_MAJOR)
  unset(SDL_MIXER_VERSION_MINOR)
  unset(SDL_MIXER_VERSION_PATCH)
endif()

set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARY})
set(SDL2_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIR})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer
                                  REQUIRED_VARS SDL2_MIXER_LIBRARIES SDL2_MIXER_INCLUDE_DIRS
                                  VERSION_VAR SDL2_MIXER_VERSION_STRING)

mark_as_advanced(SDL2_MIXER_LIBRARY SDL2_MIXER_INCLUDE_DIR)
