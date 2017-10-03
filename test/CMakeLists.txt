set(test_sources
    test_main.cpp
    test_duke_script_loader.cpp
    test_elevator.cpp
    test_physics_system.cpp
    test_player_animation_system.cpp
    test_player_attack_system.cpp
    test_spike_ball.cpp
)


add_executable(tests ${test_sources})
target_link_libraries(tests rigel_core)
target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/3rd_party/catch
)

add_test(all-tests tests)
