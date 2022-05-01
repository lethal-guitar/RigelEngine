set(sources
    loguru.hpp
    loguru.cpp
)

add_library(loguru STATIC ${sources})
target_include_directories(loguru
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(loguru PRIVATE ${CMAKE_DL_LIBS})


if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    target_compile_definitions(loguru PRIVATE "-DLOGURU_STACKTRACES=0")
endif()

rigel_disable_warnings(loguru)
