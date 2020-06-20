add_library(dear_imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/examples/imgui_impl_opengl3.cpp
    imgui/examples/imgui_impl_sdl.cpp
)
target_include_directories(dear_imgui
    PUBLIC
    ${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui
    ${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui/examples

    PRIVATE
    ${CMAKE_SOURCE_DIR}/3rd_party/glad/include
)
# target_link_libraries(dear_imgui
#     PRIVATE
#     SDL2::Core
#     glad
# )
target_link_libraries(dear_imgui
    PRIVATE
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
)

if (USE_GL_ES)
    target_compile_definitions(dear_imgui
        PRIVATE
        -DIMGUI_IMPL_OPENGL_ES2
    )
endif()

if (MSVC)
    target_compile_options(dear_imgui PRIVATE
        /w
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dear_imgui PRIVATE
        -w
    )
elseif (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(dear_imgui PRIVATE
        -w
    )
endif()
