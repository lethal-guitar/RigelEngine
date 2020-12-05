# Header-only targets
###############################################################################

add_library(catch2 INTERFACE)
target_include_directories(catch2
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/catch"
)
add_library(glm INTERFACE)
target_include_directories(glm
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/glm"
)
add_library(nlohmann-json INTERFACE)
target_include_directories(nlohmann-json
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann"
)


# Compiled targets
###############################################################################

set(ENTITYX_BUILD_SHARED false CACHE INTERNAL "")
set(ENTITYX_BUILD_TESTING false CACHE INTERNAL "")
set(ENTITYX_DT_TYPE double CACHE INTERNAL "")
set(ENTITYX_RUN_BENCHMARKS false CACHE INTERNAL "")
set(ENTITYX_MAX_COMPONENTS 64 CACHE INTERNAL "")

add_subdirectory(dbopl)
add_subdirectory(entityx)
add_subdirectory(glad)
add_subdirectory(speex_resampler)
add_subdirectory(stb)
add_subdirectory(imgui)
add_subdirectory(imgui-filebrowser)

rigel_disable_warnings(entityx)
