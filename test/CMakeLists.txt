set(test_sources
    test_main.cpp
    test_duke_script_loader.cpp
    test_elevator.cpp
    test_high_score_list.cpp
    test_json_utils.cpp
    test_letter_collection.cpp
    test_physics_system.cpp
    test_player.cpp
    test_rng.cpp
    test_spike_ball.cpp
    test_timing.cpp
)


add_executable(tests ${test_sources})
target_link_libraries(tests
    PRIVATE
    rigel_core
    catch2
)

add_test(all-tests tests)
