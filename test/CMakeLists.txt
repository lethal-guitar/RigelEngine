set(test_sources
    test_main.cpp
    test_duke_script_loader.cpp
    test_player_state_machine.cpp
)


add_executable(tests ${test_sources})
target_link_libraries(tests rigel_core)
target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/3rd_party/catch
)

add_test(all-tests tests)
