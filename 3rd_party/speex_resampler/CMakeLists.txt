set(sources
    speex/speex_resampler.h
    speex/resample.c
    speex/resample_neon.h
    speex/resample_sse.h
)


add_library(speex_resampler STATIC ${sources})
target_compile_definitions(speex_resampler PUBLIC
    OUTSIDE_SPEEX
    RANDOM_PREFIX=rigel
    FLOATING_POINT
)

if (MSVC)
    target_compile_options(speex_resampler PRIVATE
        /w
    )
elseif (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_compile_options(speex_resampler PRIVATE
        -w
    )
endif()
