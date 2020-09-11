if (USE_GL_ES)
    set(gl_variant "es2.0")
else()
    set(gl_variant "3.0")
endif()

set(sources
    "${gl_variant}/src/glad.c"
)

set(glad_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/${gl_variant}/include")

add_library(glad STATIC ${sources})
target_include_directories(glad PUBLIC ${glad_include_dir})
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
rigel_disable_warnings(glad)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(glad PRIVATE
        # This is required in order to successfully link the test executable
        # on Mac.
        -fno-common
    )
endif()
