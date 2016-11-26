cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(RigelEngine)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(main_exe_name RigelEngine)


# Dependencies
###############################################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK)
find_package(Boost 1.55 COMPONENTS program_options REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)


# Compiler settings
###############################################################################

if(MSVC)
    add_compile_options(
        /MP
        /W4
        /WX
        /wd4100 # Unused parameter
        /wd4503 # Decorated name length exceeded
        /wd4800 # Forcing value to bool
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

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
