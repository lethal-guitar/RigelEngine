add_library(imgui-filebrowser STATIC
    imfilebrowser.h
    imfilebrowser.cpp
)
target_link_libraries(imgui-filebrowser
    PUBLIC
    dear_imgui
)
target_include_directories(imgui-filebrowser
    INTERFACE
    ${CMAKE_SOURCE_DIR}/3rd_party/imgui-filebrowser
)
