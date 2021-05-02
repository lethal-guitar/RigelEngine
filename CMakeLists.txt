cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(RigelEngine)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Project settings
###############################################################################

option(USE_GL_ES "Use OpenGL ES instead of regular OpenGL" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)


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
    find_package(Boost 1.65 COMPONENTS program_options REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)
endif()

find_package(Filesystem REQUIRED COMPONENTS Final)


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

rigel_configure_compiler_warnings()
add_subdirectory(src)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    enable_testing()

    add_subdirectory(modding_tools)
    add_subdirectory(test)
endif()
