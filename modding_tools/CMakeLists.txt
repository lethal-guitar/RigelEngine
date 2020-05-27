add_executable(UserProfileTool user_profile_tool.cpp)
# target_link_libraries(UserProfileTool PRIVATE
#     SDL2::Main
#     rigel_core
# )
target_link_libraries(UserProfileTool PRIVATE
    rigel_core
)

