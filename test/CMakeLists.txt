add_executable(tests
    test_main.cpp
    test_array_view.cpp
    test_duke_script_loader.cpp
    test_elevator.cpp
    test_high_score_list.cpp
    test_json_utils.cpp
    test_letter_collection.cpp
    test_physics_system.cpp
    test_player.cpp
    test_rng.cpp
    test_spike_ball.cpp
    test_string_utils.cpp
    test_timing.cpp
)

target_link_libraries(tests
    PRIVATE
    rigel_core
    catch2
)

rigel_enable_warnings(tests)

add_test(all-tests tests)
