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
