set(sources
    dbopl.h
    dbopl.cpp
)

add_library(dbopl STATIC ${sources})

if(MSVC)
    target_compile_options(dbopl PRIVATE
        /wd4456
        /wd4244
        /wd4311
        /wd4302
        /wd4127
    )
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dbopl PRIVATE
        -w
    )
endif()
