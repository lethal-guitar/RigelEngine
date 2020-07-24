set(sources
    speex/speex_resampler.h
    speex/resample.c
    speex/resample_neon.h
    speex/resample_sse.h
)


add_library(speex_resampler STATIC ${sources})
target_include_directories(speex_resampler
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(speex_resampler PUBLIC
    OUTSIDE_SPEEX
    RANDOM_PREFIX=rigel
    FLOATING_POINT
)

rigel_disable_warnings(speex_resampler)
