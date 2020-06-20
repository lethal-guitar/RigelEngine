add_executable(UserProfileTool user_profile_tool.cpp)

# conditional compilation for emscripten (webassemly) to work
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
  target_link_libraries(UserProfileTool PRIVATE
    rigel_core
  )
else()
  target_link_libraries(UserProfileTool PRIVATE
    SDL2::Main
    rigel_core
  )      
endif()





