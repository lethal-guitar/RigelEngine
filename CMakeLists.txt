cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(RigelEngine)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(main_exe_name RigelEngine)


# Project settings
###############################################################################

option(USE_GL_ES "Use OpenGL ES instead of regular OpenGL" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)


# Dependencies
###############################################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Boost 1.67 COMPONENTS program_options REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(Filesystem)


# Compiler settings
###############################################################################

if(MSVC)
    add_compile_options(
        /Zc:__cplusplus
        /permissive-
        /MP
        /W4
        /EHsc
        /wd4100 # Unused parameter
        /wd4503 # Decorated name length exceeded
        /wd4800 # Forcing value to bool
    )
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)

    if (WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Weverything
        -Wno-unknown-warning-option
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
        -Wno-exit-time-destructors
        -Wno-missing-braces
        -Wno-padded
        -Wno-sign-conversion
        -Wno-switch-enum
        -Wno-unused-parameter
        -Wno-unused-lambda-capture
        -Wno-weak-vtables
        -Wno-global-constructors

        -fcolor-diagnostics
    )

    if (WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(
        -Wall
        -Wextra
        -pedantic
        -Wno-maybe-uninitialized
        -Wno-unused-parameter
    )

    if (WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
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

add_subdirectory(3rd_party/dbopl)
add_subdirectory(3rd_party/entityx)
add_subdirectory(3rd_party/glad)
add_subdirectory(3rd_party/speex_resampler)
add_subdirectory(3rd_party/imgui)

target_compile_options(entityx PRIVATE
    -w
)


# RigelEngine build targets
###############################################################################

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
