Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests
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
    Catch2::Catch2WithMain
)

rigel_enable_warnings(tests)

# Ignore warnings emitted from Catch macros
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(tests
        PRIVATE
        -Wno-parentheses
    )
endif()

add_test(all-tests tests)
