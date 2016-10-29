cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(RigelEngine)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

set(main_exe_name RigelEngine)


# Dependencies
###############################################################################

find_package(Boost 1.58 COMPONENTS program_options REQUIRED)


include(FindPkgConfig)

if (PKG_CONFIG_FOUND)
    pkg_search_module(SDL2 REQUIRED sdl2)
    pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer>=2.0.0)
else()
    # TODO: This is a HACK for Windows until we have a proper find script.
    # It uses the SDL1 find script, so you can set the locations manually in
    # cmake-gui
    find_package(SDL QUIET)
    find_package(SDL_mixer QUIET)

    set(SDL2_INCLUDE_DIR ${SDL_INCLUDE_DIR})
    set(SDL2_MIXER_INCLUDE_DIRS ${SDL_MIXER_INCLUDE_DIRS})
    set(SDL2_LIBRARY ${SDL_LIBRARY})
    set(SDL2_MIXER_LIBRARIES ${SDL_MIXER_LIBRARIES})
    set(SDLMAIN2_LIBRARY ${SDLMAIN_LIBRARY})
endif()


# Compiler settings
###############################################################################

if(MSVC)
    add_compile_options(
        /MP
        /W4
        /WX
        /wd4100 # Unused parameter
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
        -Wno-exit-time-destructors
        -Wno-missing-braces
        -Wno-padded
        -Wno-shadow-field-in-constructor
        -Wno-sign-conversion
        -Wno-switch-enum
        -Wno-unused-parameter
        -Wno-weak-vtables

        -fcolor-diagnostics
    )
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(
        -Werror
        -Wall
        -Wextra
        -pedantic
        -Wno-maybe-uninitialized
        -Wno-unused-parameter
    )
else()
    message(FATAL_ERROR "Unrecognized compiler")
endif()


# 3rd party build targets
###############################################################################

set(ENTITYX_BUILD_SHARED false CACHE INTERNAL "")
set(ENTITYX_BUILD_TESTING false CACHE INTERNAL "")
set(ENTITYX_DT_TYPE double CACHE INTERNAL "")
set(ENTITYX_RUN_BENCHMARKS false CACHE INTERNAL "")
set(ENTITYX_MAX_COMPONENTS 64 CACHE INTERNAL "")

add_subdirectory(3rd_party/entityx)
add_subdirectory(3rd_party/dbopl)
add_subdirectory(3rd_party/speex_resampler)

target_compile_options(entityx PRIVATE
    -w
)


# RigelEngine build targets
###############################################################################

add_subdirectory(src)
add_subdirectory(test)
