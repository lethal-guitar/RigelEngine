add_executable(UserProfileTool user_profile_tool.cpp)
target_link_libraries(UserProfileTool PRIVATE
    SDL2::Main
    rigel_core
)
rigel_enable_warnings(UserProfileTool)
