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

set(core_sources
    src/base/grid.hpp
    src/base/spatial_types.hpp
    src/base/warnings.hpp
    src/data/audio_buffer.hpp
    src/data/duke_script.hpp
    src/data/game_session_data.hpp
    src/data/game_traits.cpp
    src/data/game_traits.hpp
    src/data/image.cpp
    src/data/image.hpp
    src/data/map.cpp
    src/data/map.hpp
    src/data/player_data.hpp
    src/data/sound_ids.hpp
    src/data/tile_set.cpp
    src/data/tile_set.hpp
    src/data/unit_conversions.cpp
    src/data/unit_conversions.hpp
    src/engine/base_components.hpp
    src/engine/map_renderer.cpp
    src/engine/map_renderer.hpp
    src/engine/physics_system.cpp
    src/engine/physics_system.hpp
    src/engine/rendering_system.cpp
    src/engine/rendering_system.hpp
    src/engine/sound_system.cpp
    src/engine/sound_system.hpp
    src/engine/tile_renderer.cpp
    src/engine/tile_renderer.hpp
    src/engine/timing.cpp
    src/engine/timing.hpp
    src/game_logic/collectable_components.hpp
    src/game_logic/entity_factory.cpp
    src/game_logic/entity_factory.hpp
    src/game_logic/player_control_system.cpp
    src/game_logic/player_control_system.hpp
    src/game_logic/player_interaction_system.cpp
    src/game_logic/player_interaction_system.hpp
    src/loader/actor_image_package.cpp
    src/loader/actor_image_package.hpp
    src/loader/audio_package.cpp
    src/loader/audio_package.hpp
    src/loader/bitwise_iter.hpp
    src/loader/byte_buffer.hpp
    src/loader/cmp_file_package.cpp
    src/loader/cmp_file_package.hpp
    src/loader/duke_script_loader.cpp
    src/loader/duke_script_loader.hpp
    src/loader/ega_image_decoder.cpp
    src/loader/ega_image_decoder.hpp
    src/loader/file_utils.cpp
    src/loader/file_utils.hpp
    src/loader/level_loader.cpp
    src/loader/level_loader.hpp
    src/loader/movie_loader.cpp
    src/loader/movie_loader.hpp
    src/loader/music_loader.cpp
    src/loader/music_loader.hpp
    src/loader/palette.cpp
    src/loader/palette.hpp
    src/loader/resource_bundle.cpp
    src/loader/resource_bundle.hpp
    src/loader/rle_compression.hpp
    src/loader/voc_decoder.cpp
    src/loader/voc_decoder.hpp
    src/sdl_utils/error.cpp
    src/sdl_utils/error.hpp
    src/sdl_utils/ptr.hpp
    src/sdl_utils/texture.cpp
    src/sdl_utils/texture.hpp
    src/ui/apogee_logo.cpp
    src/ui/apogee_logo.hpp
    src/ui/bonus_screen.cpp
    src/ui/bonus_screen.hpp
    src/ui/duke_script_runner.cpp
    src/ui/duke_script_runner.hpp
    src/ui/hud_renderer.cpp
    src/ui/hud_renderer.hpp
    src/ui/intro_movie.cpp
    src/ui/intro_movie.hpp
    src/ui/menu_element_renderer.cpp
    src/ui/menu_element_renderer.hpp
    src/ui/movie_player.cpp
    src/ui/movie_player.hpp
    src/ui/utils.cpp
    src/ui/utils.hpp
    src/utils/container_tools.hpp
    src/utils/enum_hash.hpp
    src/utils/math_tools.hpp

    src/game.cpp
    src/game.hpp
    src/game_mode.hpp
    src/ingame_mode.cpp
    src/ingame_mode.hpp
    src/intro_demo_loop_mode.cpp
    src/intro_demo_loop_mode.hpp
    src/menu_mode.cpp
    src/menu_mode.hpp
    src/mode_stage.hpp
)


# Core library
set(core_lib_name rigel_core)

add_library(${core_lib_name} STATIC ${core_sources})
target_include_directories(${core_lib_name}
    PUBLIC
    src
    "${SDL2_INCLUDE_DIR}"
    ${SDL2_MIXER_INCLUDE_DIRS}
    3rd_party/entityx

    PRIVATE
    3rd_party/atria/src
    3rd_party/atria/compat
    3rd_party/dbopl
    3rd_party/speex_resampler
)
target_link_libraries(${core_lib_name}
    PUBLIC
    Boost::boost
    "${SDL2_LIBRARY}"
    ${SDL2_MIXER_LIBRARIES}

    PRIVATE
    entityx
    dbopl
    speex_resampler
)


# Main executable
set(main_exe_sources
    src/main.cpp
)


add_executable(${main_exe_name} ${main_exe_sources})
target_link_libraries(${main_exe_name} PRIVATE
    ${SDLMAIN2_LIBRARY}
    ${core_lib_name}
    Boost::program_options
    Boost::disable_autolinking
    Boost::dynamic_linking
)

set(test_sources
    test/test_main.cpp
    test/test_duke_script_loader.cpp
    test/test_player_state_machine.cpp
)


# Test executable
set(test_exe_name tests)

add_executable(${test_exe_name} ${test_sources})
target_link_libraries(${test_exe_name}
    ${core_lib_name}
)
target_include_directories(${test_exe_name} PRIVATE
    3rd_party/catch
)
