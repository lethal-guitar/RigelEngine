include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.5.3
)
FetchContent_GetProperties(benchmark)
if(NOT benchmark_POPULATED)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    FetchContent_Populate(benchmark)
    add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})
endif()

add_executable(benchmarks
    bench_string_utils.cpp
)

target_link_libraries(benchmarks PRIVATE
    rigel_core
    benchmark::benchmark_main
)
