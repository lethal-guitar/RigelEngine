set(core_sources
    assets/actor_image_package.cpp
    assets/actor_image_package.hpp
    assets/audio_package.cpp
    assets/audio_package.hpp
    assets/bitwise_iter.hpp
    assets/byte_buffer.hpp
    assets/cmp_file_package.cpp
    assets/cmp_file_package.hpp
    assets/duke_script_loader.cpp
    assets/duke_script_loader.hpp
    assets/ega_image_decoder.cpp
    assets/ega_image_decoder.hpp
    assets/file_utils.cpp
    assets/file_utils.hpp
    assets/level_loader.cpp
    assets/level_loader.hpp
    assets/movie_loader.cpp
    assets/movie_loader.hpp
    assets/music_loader.cpp
    assets/music_loader.hpp
    assets/palette.cpp
    assets/palette.hpp
    assets/png_image.cpp
    assets/png_image.hpp
    assets/resource_loader.cpp
    assets/resource_loader.hpp
    assets/rle_compression.hpp
    assets/ultrawide_hud_image.ipp
    assets/user_profile_import.cpp
    assets/user_profile_import.hpp
    assets/voc_decoder.cpp
    assets/voc_decoder.hpp
    assets/wide_hud_image.ipp
    audio/adlib_emulator.hpp
    audio/software_imf_player.cpp
    audio/software_imf_player.hpp
    audio/sound_system.cpp
    audio/sound_system.hpp
    base/array_view.cpp
    base/array_view.hpp
    base/audio_buffer.hpp
    base/clock.hpp
    base/container_utils.hpp
    base/defer.hpp
    base/grid.hpp
    base/image.cpp
    base/image.hpp
    base/math_utils.hpp
    base/spatial_types.hpp
    base/static_vector.hpp
    base/string_utils.cpp
    base/string_utils.hpp
    base/warnings.hpp
    data/actor_ids.hpp
    data/bonus.hpp
    data/duke_script.hpp
    data/game_options.cpp
    data/game_options.hpp
    data/game_session_data.hpp
    data/game_traits.hpp
    data/high_score_list.cpp
    data/high_score_list.hpp
    data/level_hints.cpp
    data/level_hints.hpp
    data/map.cpp
    data/map.hpp
    data/mod_library.cpp
    data/mod_library.hpp
    data/player_model.cpp
    data/player_model.hpp
    data/saved_game.hpp
    data/song.hpp
    data/sound_ids.hpp
    data/strings.hpp
    data/tile_attributes.hpp
    data/tile_attributes.ipp
    data/tutorial_messages.cpp
    data/tutorial_messages.hpp
    data/unit_conversions.cpp
    data/unit_conversions.hpp
    engine/base_components.hpp
    engine/collision_checker.cpp
    engine/collision_checker.hpp
    engine/entity_activation_system.cpp
    engine/entity_activation_system.hpp
    engine/entity_tools.hpp
    engine/graphical_effects.cpp
    engine/graphical_effects.hpp
    engine/isprite_factory.hpp
    engine/life_time_components.hpp
    engine/life_time_system.cpp
    engine/life_time_system.hpp
    engine/map_renderer.cpp
    engine/map_renderer.hpp
    engine/motion_smoothing.hpp
    engine/movement.cpp
    engine/movement.hpp
    engine/particle_system.cpp
    engine/particle_system.hpp
    engine/physical_components.hpp
    engine/physics.cpp
    engine/physics.hpp
    engine/physics_system.cpp
    engine/physics_system.hpp
    engine/random_number_generator.cpp
    engine/random_number_generator.hpp
    engine/sprite_factory.cpp
    engine/sprite_factory.hpp
    engine/sprite_rendering_system.cpp
    engine/sprite_rendering_system.hpp
    engine/sprite_tools.hpp
    engine/tiled_texture.cpp
    engine/tiled_texture.hpp
    engine/timing.hpp
    engine/visual_components.hpp
    frontend/anti_piracy_screen_mode.cpp
    frontend/anti_piracy_screen_mode.hpp
    frontend/command_line_options.hpp
    frontend/demo_player.cpp
    frontend/demo_player.hpp
    frontend/game.cpp
    frontend/game.hpp
    frontend/game_mode.cpp
    frontend/game_mode.hpp
    frontend/game_runner.cpp
    frontend/game_runner.hpp
    frontend/game_service_provider.hpp
    frontend/game_session_mode.cpp
    frontend/game_session_mode.hpp
    frontend/input_handler.cpp
    frontend/input_handler.hpp
    frontend/intro_demo_loop_mode.cpp
    frontend/intro_demo_loop_mode.hpp
    frontend/json_utils.cpp
    frontend/json_utils.hpp
    frontend/menu_mode.cpp
    frontend/menu_mode.hpp
    frontend/user_profile.cpp
    frontend/user_profile.hpp
    game_logic/behavior_controller.hpp
    game_logic/behavior_controller_system.cpp
    game_logic/behavior_controller_system.hpp
    game_logic/camera.cpp
    game_logic/camera.hpp
    game_logic/collectable_components.hpp
    game_logic/damage_components.hpp
    game_logic/damage_infliction_system.cpp
    game_logic/damage_infliction_system.hpp
    game_logic/debugging_system.cpp
    game_logic/debugging_system.hpp
    game_logic/dynamic_geometry_components.hpp
    game_logic/dynamic_geometry_system.cpp
    game_logic/dynamic_geometry_system.hpp
    game_logic/earth_quake_effect.cpp
    game_logic/earth_quake_effect.hpp
    game_logic/effect_actor_components.cpp
    game_logic/effect_actor_components.hpp
    game_logic/effects_system.cpp
    game_logic/effects_system.hpp
    game_logic/enemies/big_green_cat.cpp
    game_logic/enemies/big_green_cat.hpp
    game_logic/enemies/blue_guard.cpp
    game_logic/enemies/blue_guard.hpp
    game_logic/enemies/bomber_plane.cpp
    game_logic/enemies/bomber_plane.hpp
    game_logic/enemies/boss_episode_1.cpp
    game_logic/enemies/boss_episode_1.hpp
    game_logic/enemies/boss_episode_2.cpp
    game_logic/enemies/boss_episode_2.hpp
    game_logic/enemies/boss_episode_3.cpp
    game_logic/enemies/boss_episode_3.hpp
    game_logic/enemies/boss_episode_4.cpp
    game_logic/enemies/boss_episode_4.hpp
    game_logic/enemies/ceiling_sucker.cpp
    game_logic/enemies/ceiling_sucker.hpp
    game_logic/enemies/dying_boss.cpp
    game_logic/enemies/dying_boss.hpp
    game_logic/enemies/enemy_rocket.cpp
    game_logic/enemies/enemy_rocket.hpp
    game_logic/enemies/eyeball_thrower.cpp
    game_logic/enemies/eyeball_thrower.hpp
    game_logic/enemies/flame_thrower_bot.cpp
    game_logic/enemies/flame_thrower_bot.hpp
    game_logic/enemies/floating_laser_bot.cpp
    game_logic/enemies/floating_laser_bot.hpp
    game_logic/enemies/grabber_claw.cpp
    game_logic/enemies/grabber_claw.hpp
    game_logic/enemies/green_bird.cpp
    game_logic/enemies/green_bird.hpp
    game_logic/enemies/hover_bot.cpp
    game_logic/enemies/hover_bot.hpp
    game_logic/enemies/laser_turret.cpp
    game_logic/enemies/laser_turret.hpp
    game_logic/enemies/messenger_drone.cpp
    game_logic/enemies/messenger_drone.hpp
    game_logic/enemies/prisoner.cpp
    game_logic/enemies/prisoner.hpp
    game_logic/enemies/red_bird.cpp
    game_logic/enemies/red_bird.hpp
    game_logic/enemies/rigelatin_soldier.cpp
    game_logic/enemies/rigelatin_soldier.hpp
    game_logic/enemies/rocket_turret.cpp
    game_logic/enemies/rocket_turret.hpp
    game_logic/enemies/security_camera.cpp
    game_logic/enemies/security_camera.hpp
    game_logic/enemies/simple_walker.cpp
    game_logic/enemies/simple_walker.hpp
    game_logic/enemies/slime_blob.cpp
    game_logic/enemies/slime_blob.hpp
    game_logic/enemies/small_flying_ship.cpp
    game_logic/enemies/small_flying_ship.hpp
    game_logic/enemies/snake.cpp
    game_logic/enemies/snake.hpp
    game_logic/enemies/spider.cpp
    game_logic/enemies/spider.hpp
    game_logic/enemies/spike_ball.cpp
    game_logic/enemies/spike_ball.hpp
    game_logic/enemies/spiked_green_creature.cpp
    game_logic/enemies/spiked_green_creature.hpp
    game_logic/enemies/unicycle_bot.cpp
    game_logic/enemies/unicycle_bot.hpp
    game_logic/enemies/wall_walker.cpp
    game_logic/enemies/wall_walker.hpp
    game_logic/enemies/watch_bot.cpp
    game_logic/enemies/watch_bot.hpp
    game_logic/entity_configuration.ipp
    game_logic/entity_factory.cpp
    game_logic/entity_factory.hpp
    game_logic/game_world.cpp
    game_logic/game_world.hpp
    game_logic/global_dependencies.hpp
    game_logic/hazards/lava_fountain.cpp
    game_logic/hazards/lava_fountain.hpp
    game_logic/hazards/slime_pipe.cpp
    game_logic/hazards/slime_pipe.hpp
    game_logic/hazards/smash_hammer.cpp
    game_logic/hazards/smash_hammer.hpp
    game_logic/ientity_factory.hpp
    game_logic/interactive/blowing_fan.cpp
    game_logic/interactive/blowing_fan.hpp
    game_logic/interactive/elevator.cpp
    game_logic/interactive/elevator.hpp
    game_logic/interactive/enemy_radar.cpp
    game_logic/interactive/enemy_radar.hpp
    game_logic/interactive/force_field.cpp
    game_logic/interactive/force_field.hpp
    game_logic/interactive/item_container.cpp
    game_logic/interactive/item_container.hpp
    game_logic/interactive/locked_door.cpp
    game_logic/interactive/locked_door.hpp
    game_logic/interactive/missile.cpp
    game_logic/interactive/missile.hpp
    game_logic/interactive/respawn_checkpoint.cpp
    game_logic/interactive/respawn_checkpoint.hpp
    game_logic/interactive/sliding_door.cpp
    game_logic/interactive/sliding_door.hpp
    game_logic/interactive/super_force_field.cpp
    game_logic/interactive/super_force_field.hpp
    game_logic/interactive/tile_burner.cpp
    game_logic/interactive/tile_burner.hpp
    game_logic/player.cpp
    game_logic/player.hpp
    game_logic/player/components.hpp
    game_logic/player/damage_system.cpp
    game_logic/player/damage_system.hpp
    game_logic/player/interaction_system.cpp
    game_logic/player/interaction_system.hpp
    game_logic/player/level_exit_trigger.cpp
    game_logic/player/level_exit_trigger.hpp
    game_logic/player/projectile_system.cpp
    game_logic/player/projectile_system.hpp
    game_logic/player/ship.cpp
    game_logic/player/ship.hpp
    game_logic/world_state.cpp
    game_logic/world_state.hpp
    game_logic_classic/actors.c
    game_logic_classic/actors.h
    game_logic_classic/game.h
    game_logic_classic/game1.c
    game_logic_classic/game2.c
    game_logic_classic/game3.c
    game_logic_classic/game_world_classic.cpp
    game_logic_classic/game_world_classic.hpp
    game_logic_classic/memory.c
    game_logic_classic/misc.c
    game_logic_classic/particls.c
    game_logic_classic/player.c
    game_logic_classic/sprite.c
    game_logic_classic/types.h
    game_logic_common/igame_world.hpp
    game_logic_common/input.hpp
    game_logic_common/utils.hpp
    renderer/custom_quad_batch.cpp
    renderer/custom_quad_batch.hpp
    renderer/fps_limiter.cpp
    renderer/fps_limiter.hpp
    renderer/opengl.cpp
    renderer/opengl.hpp
    renderer/renderer.cpp
    renderer/renderer.hpp
    renderer/renderer_support.hpp
    renderer/shader.cpp
    renderer/shader.hpp
    renderer/shader_code.cpp
    renderer/shader_code.hpp
    renderer/texture.cpp
    renderer/texture.hpp
    renderer/texture_atlas.cpp
    renderer/texture_atlas.hpp
    renderer/upscaling.cpp
    renderer/upscaling.hpp
    renderer/vertex_buffer_utils.hpp
    renderer/viewport_utils.cpp
    renderer/viewport_utils.hpp
    sdl_utils/error.cpp
    sdl_utils/error.hpp
    sdl_utils/key_code.hpp
    sdl_utils/platform.cpp
    sdl_utils/platform.hpp
    sdl_utils/ptr.hpp
    ui/apogee_logo.cpp
    ui/apogee_logo.hpp
    ui/bonus_screen.cpp
    ui/bonus_screen.hpp
    ui/credits.cpp
    ui/credits.hpp
    ui/duke_3d_teaser_screen.cpp
    ui/duke_3d_teaser_screen.hpp
    ui/duke_script_runner.cpp
    ui/duke_script_runner.hpp
    ui/episode_end_sequence.cpp
    ui/episode_end_sequence.hpp
    ui/fps_display.cpp
    ui/fps_display.hpp
    ui/game_path_browser.cpp
    ui/game_path_browser.hpp
    ui/high_score_list.cpp
    ui/high_score_list.hpp
    ui/hud_renderer.cpp
    ui/hud_renderer.hpp
    ui/imgui_integration.cpp
    ui/imgui_integration.hpp
    ui/ingame_menu.cpp
    ui/ingame_menu.hpp
    ui/ingame_message_display.cpp
    ui/ingame_message_display.hpp
    ui/intro_movie.cpp
    ui/intro_movie.hpp
    ui/menu_element_renderer.cpp
    ui/menu_element_renderer.hpp
    ui/menu_navigation.cpp
    ui/menu_navigation.hpp
    ui/movie_player.cpp
    ui/movie_player.hpp
    ui/options_menu.cpp
    ui/options_menu.hpp
    ui/text_entry_widget.cpp
    ui/text_entry_widget.hpp
    ui/utils.cpp
    ui/utils.hpp
    platform.cpp
    platform.hpp
)

# Core library
set(RIGEL_COMMIT_HASH "N/A")
rigel_determine_git_commit_hash(RIGEL_COMMIT_HASH)

set(RIGEL_IS_CUSTOM_BUILD "true")
rigel_check_if_custom_build(RIGEL_IS_CUSTOM_BUILD)

configure_file(
    version_info.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_info.hpp"
)

add_library(rigel_core STATIC ${core_sources})
target_include_directories(rigel_core
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(rigel_core
    PUBLIC
    SDL2::Core
    SDL2::Mixer
    dear_imgui
    entityx
    glad
    glm
    imgui-filebrowser
    nlohmann-json
    std::filesystem
    loguru

    PRIVATE
    dbopl
    nuked-opl3
    speex_resampler
    static_vector
    stb
)

rigel_enable_warnings(rigel_core)

if(USE_GL_ES)
    target_compile_definitions(rigel_core PUBLIC
        RIGEL_USE_GL_ES=1
    )
endif()


# Main executable
set(icon_file_osx "${CMAKE_SOURCE_DIR}/dist/osx/RigelEngine.icns")
set_source_files_properties(${icon_file_osx} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

set(rc_file_windows "${CMAKE_SOURCE_DIR}/dist/win/RigelEngine.rc")

add_executable(RigelEngine WIN32 MACOSX_BUNDLE
    ${icon_file_osx}
    ${rc_file_windows}
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    target_sources(RigelEngine PRIVATE emscripten_main.cpp)
    target_link_libraries(RigelEngine PRIVATE
      rigel_core
    )
    target_link_options(RigelEngine PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s INITIAL_MEMORY=64mb"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s MIN_WEBGL_VERSION=1"
        "SHELL:-s MAX_WEBGL_VERSION=2"
        "SHELL:-s FULL_ES2=1"
        "SHELL:--preload-file \"${WEBASSEMBLY_GAME_PATH}/@/duke\""
        "SHELL:--shell-file \"${CMAKE_SOURCE_DIR}/dist/emscripten/shell.html\""
        --no-heap-copy
    )
    set_target_properties(RigelEngine PROPERTIES
        SUFFIX
        ".html"
    )
else()
    target_sources(RigelEngine PRIVATE
        game_main.cpp
        game_main.hpp
        main.cpp
    )
    target_link_libraries(RigelEngine PRIVATE
        SDL2::Main
        rigel_core
        lyra
    )

    set_target_properties(RigelEngine PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/dist/osx/Info.plist
    )
endif()

rigel_enable_warnings(RigelEngine)

if(APPLE)
    install(TARGETS RigelEngine
        DESTINATION .
        COMPONENT bin)
    install(CODE [[
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS TRUE)
        fixup_bundle("${CMAKE_INSTALL_PREFIX}/RigelEngine.app" "" "")
    ]] COMPONENT bin)
elseif(UNIX)
    include(GNUInstallDirs)
    install(TARGETS RigelEngine
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT bin
    )
    install(FILES ${CMAKE_SOURCE_DIR}/dist/linux/rigelengine.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(FILES ${CMAKE_SOURCE_DIR}/dist/linux/rigelengine.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
    )
    install(FILES ${CMAKE_SOURCE_DIR}/dist/linux/rigelengine_48.png
        RENAME rigelengine.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
    )
    install(FILES ${CMAKE_SOURCE_DIR}/dist/linux/rigelengine_64.png
        RENAME rigelengine.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
    )
    install(FILES ${CMAKE_SOURCE_DIR}/dist/linux/rigelengine_128.png
        RENAME rigelengine.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    )
endif()
