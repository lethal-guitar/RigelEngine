set(sources
    dbopl.h
    dbopl.cpp
)

add_library(dbopl STATIC ${sources})
rigel_disable_warnings(dbopl)
