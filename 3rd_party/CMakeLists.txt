# Header-only targets
###############################################################################

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
add_library(static_vector INTERFACE)
target_include_directories(static_vector
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/static_vector/include"
)
add_library(lyra INTERFACE)
target_include_directories(lyra
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/lyra/include"
)


# Compiled targets
###############################################################################

set(ENTITYX_BUILD_SHARED false CACHE INTERNAL "")
set(ENTITYX_BUILD_TESTING false CACHE INTERNAL "")
set(ENTITYX_DT_TYPE double CACHE INTERNAL "")
set(ENTITYX_RUN_BENCHMARKS false CACHE INTERNAL "")
set(ENTITYX_MAX_COMPONENTS 64 CACHE INTERNAL "")

# EXCLUDE_FROM_ALL to disable installing these HO or static library deps
add_subdirectory(dbopl EXCLUDE_FROM_ALL)
add_subdirectory(entityx EXCLUDE_FROM_ALL)
add_subdirectory(glad EXCLUDE_FROM_ALL)
add_subdirectory(speex_resampler EXCLUDE_FROM_ALL)
add_subdirectory(stb EXCLUDE_FROM_ALL)
add_subdirectory(imgui EXCLUDE_FROM_ALL)
add_subdirectory(imgui-filebrowser EXCLUDE_FROM_ALL)
add_subdirectory(nuked-opl3 EXCLUDE_FROM_ALL)
add_subdirectory(loguru EXCLUDE_FROM_ALL)

rigel_disable_warnings(entityx)
