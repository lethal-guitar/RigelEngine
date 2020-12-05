set(sources
    stb_image.h
    stb_image_write.h
    stb_rect_pack.h
    stb.cpp
)

add_library(stb STATIC ${sources})
target_include_directories(stb
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
rigel_disable_warnings(stb)

