set(sources
    dbopl.h
    dbopl.cpp
)

add_library(dbopl STATIC ${sources})
target_include_directories(dbopl
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
rigel_disable_warnings(dbopl)
