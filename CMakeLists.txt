cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(RigelEngine VERSION 0.8.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Project settings
###############################################################################

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  message(STATUS "No build type was specified, will default to ${CMAKE_BUILD_TYPE}")
endif()

option(USE_GL_ES "Use OpenGL ES instead of regular OpenGL" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_MODDING_TOOLS "Build modding tools" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Dependencies
###############################################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include("${CMAKE_SOURCE_DIR}/cmake/rigel.cmake")

# This catches the most common case, where none of the submodules have been
# initialized. It's still possible to get in a state where entityx is present
# but other submodules are not, but that's unlikely to happen to someone who
# has just cloned the repo and wants to build for the first time.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/3rd_party/entityx/entityx/Entity.h")
    message(FATAL_ERROR
        "It seems git submodules were not initialized. You need to run git submodule update --init --recursive, and then re-run CMake.")
endif()


if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(WEBASSEMBLY_GAME_PATH "" CACHE PATH "Path to folder containing Duke Nukem II files")
    set(USE_GL_ES ON)

    if (NOT WEBASSEMBLY_GAME_PATH)
        message(FATAL_ERROR
            "WEBASSEMBLY_GAME_PATH not defined. This is required for a Webassembly build. Point it to a folder containing Duke Nukem II data files. Remove any trailing '/' from the path.")
    else()
        message(STATUS "Bundling game data from path: ${WEBASSEMBLY_GAME_PATH}")
    endif()

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        # Emscripten's CMake toolchain defaults to O2, but we want O3
        add_compile_options(-O3)
    endif()

    rigel_define_wasm_targets_for_dependencies()
else()
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost 1.65 COMPONENTS program_options)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)
endif()

find_package(Filesystem REQUIRED COMPONENTS Final)
find_package(Git)


# Compiler settings
###############################################################################

if(MSVC)
    add_compile_options(
        /Zc:__cplusplus
        /permissive-
        /MP
        /EHsc
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -fcolor-diagnostics
    )
endif()


# Build targets
###############################################################################

add_subdirectory(3rd_party)
add_subdirectory(src)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    if(BUILD_MODDING_TOOLS)
        add_subdirectory(modding_tools)
    endif()

    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(test)
    endif()
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/rigel_pack.cmake)

if(NOT ${Boost_FOUND})
    message(WARNING "Boost not found, building without command line argument support")
endif()
