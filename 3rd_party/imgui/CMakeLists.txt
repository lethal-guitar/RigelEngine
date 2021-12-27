add_library(dear_imgui STATIC
    imgui_impl_opengl3.cpp
    imgui_impl_sdl.cpp

    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)
target_include_directories(dear_imgui
    PUBLIC
    ${CMAKE_SOURCE_DIR}/3rd_party/imgui/
    ${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui
)
target_link_libraries(dear_imgui
    PRIVATE
    SDL2::Core
    stb
    glad
)

if (APPLE)
    target_link_libraries(dear_imgui
        PRIVATE
        "-framework AppKit"
    )
endif()


target_compile_definitions(dear_imgui
    PRIVATE
    -DIMGUI_IMPL_OPENGL_LOADER_GLAD
    -DIMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
    "-DIMGUI_STB_RECT_PACK_FILENAME=<stb_rect_pack.h>"
)

if (USE_GL_ES)
    target_compile_definitions(dear_imgui
        PRIVATE
        -DIMGUI_IMPL_OPENGL_ES2
        -DIMGUI_IMPL_OPENGL_ES_USE_LOADER
    )
endif()

rigel_disable_warnings(dear_imgui)
