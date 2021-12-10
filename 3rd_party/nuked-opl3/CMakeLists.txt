set(sources
    opl3.h
    opl3.c
)

add_library(nuked-opl3 STATIC ${sources})
target_include_directories(nuked-opl3
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
rigel_disable_warnings(nuked-opl3)
