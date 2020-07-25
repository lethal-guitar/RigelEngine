# Emscripten provides its own versions of the SDL and Boost libraries.
# Calling this function defines the same targets that would normally be
# defined by the corresponding find_package calls, but these targets
# set the appropriate compiler and linker flags for Emscripten instead of
# referring to external libraries.
function(rigel_define_wasm_targets_for_dependencies)
    add_library(SDL2::Core INTERFACE IMPORTED)
    set_target_properties(SDL2::Core PROPERTIES
        INTERFACE_COMPILE_OPTIONS
        "SHELL:-s USE_SDL=2"
        INTERFACE_LINK_OPTIONS
        "SHELL:-s USE_SDL=2"
    )

    add_library(SDL2::Mixer INTERFACE IMPORTED)
    set_target_properties(SDL2::Mixer PROPERTIES
        INTERFACE_COMPILE_OPTIONS
        "SHELL:-s USE_SDL_MIXER=2"
        INTERFACE_LINK_OPTIONS
        "SHELL:-s USE_SDL_MIXER=2"
    )

    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES
        INTERFACE_COMPILE_OPTIONS
        "SHELL:-s USE_BOOST_HEADERS=1"
    )
endfunction()


function(rigel_configure_compiler_warnings)
    if(MSVC)
        add_compile_options(
            /W4
            /wd4100 # Unused parameter
            /wd4503 # Decorated name length exceeded
            /wd4800 # Forcing value to bool
        )
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)

        if (WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -Weverything
            -Wno-unknown-warning-option
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-covered-switch-default
            -Wno-exit-time-destructors
            -Wno-missing-braces
            -Wno-padded
            -Wno-sign-conversion
            -Wno-switch-enum
            -Wno-unused-parameter
            -Wno-unused-lambda-capture
            -Wno-weak-vtables
            -Wno-global-constructors
            -Wno-float-equal
            -Wno-double-promotion
        )

        if (WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        add_compile_options(
            -Wall
            -Wextra
            -pedantic
            -Wno-maybe-uninitialized
            -Wno-unused-parameter
        )

        if (WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    else()
        message(FATAL_ERROR "Unrecognized compiler")
    endif()
endfunction()


function(rigel_disable_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
            /w
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE
            -w
        )
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PRIVATE
            -w
        )
    else()
        message(FATAL_ERROR "Unrecognized compiler")
    endif()
endfunction()
