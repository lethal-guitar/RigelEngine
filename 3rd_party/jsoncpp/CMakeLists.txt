set(sources
    jsoncpp.cpp
)

set(jsoncpp_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(jsoncpp STATIC ${sources})
target_include_directories(jsoncpp PUBLIC ${jsoncpp_include_dir})

if (MSVC)
    target_compile_options(jsoncpp PRIVATE /w)
else()
    target_compile_options(jsoncpp PRIVATE -w)
endif()
