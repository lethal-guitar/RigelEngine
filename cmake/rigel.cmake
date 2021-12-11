# Emscripten provides its own versions of the SDL libraries.
# Calling this function defines the same targets that would normally be
# defined by the corresponding find_package calls, but these targets
# set the appropriate compiler and linker flags for Emscripten instead of
# referring to external libraries.
function(rigel_define_wasm_targets_for_dependencies)
    add_library(SDL2::Core INTERFACE IMPORTED)
    target_compile_options(SDL2::Core INTERFACE "SHELL:-s USE_SDL=2")
    target_link_options(SDL2::Core INTERFACE "SHELL:-s USE_SDL=2")

    add_library(SDL2::Mixer INTERFACE IMPORTED)
    target_compile_options(SDL2::Mixer INTERFACE "SHELL:-s USE_SDL_MIXER=2")
    target_link_options(SDL2::Mixer INTERFACE "SHELL:-s USE_SDL_MIXER=2")
endfunction()


function(rigel_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /wd4100 # Unused parameter
            /wd4503 # Decorated name length exceeded
            /wd4800 # Forcing value to bool
        )
        add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)

        if (WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE
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
            -Wno-unused-private-field
            -Wno-weak-vtables
            -Wno-global-constructors
            -Wno-float-equal
            -Wno-double-promotion
            -Wno-poison-system-directories
            -Wno-ctad-maybe-unsupported
            -Wno-implicit-int-float-conversion
            -Wno-suggest-destructor-override
            -Wno-return-std-move-in-c++11
            -Wno-non-virtual-dtor
        )

        if (WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -pedantic
            -Wno-maybe-uninitialized
            -Wno-unused-parameter
            -Wno-psabi
        )

        if (WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    else()
        message(FATAL_ERROR "Unrecognized compiler")
    endif()
endfunction()


function(rigel_disable_warnings target)
    # MSVC accepts `-w` in addition to `/w`, so we can use `-w` for all
    # compilers.
    target_compile_options(${target} PRIVATE -w)
endfunction()


function(rigel_determine_git_commit_hash output_var)
    if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git/")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short=10 HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE git_exit_code
            OUTPUT_VARIABLE git_commit_hash
        )
        if(${git_exit_code} EQUAL 0)
            string(STRIP "${git_commit_hash}" git_commit_hash)
            set(${output_var} "${git_commit_hash}" PARENT_SCOPE)
        endif()
    endif()
endfunction()
