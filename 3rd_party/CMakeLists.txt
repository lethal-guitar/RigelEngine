set(ENTITYX_BUILD_SHARED false CACHE INTERNAL "")
set(ENTITYX_BUILD_TESTING false CACHE INTERNAL "")
set(ENTITYX_DT_TYPE double CACHE INTERNAL "")
set(ENTITYX_RUN_BENCHMARKS false CACHE INTERNAL "")
set(ENTITYX_MAX_COMPONENTS 64 CACHE INTERNAL "")

add_subdirectory(dbopl)
add_subdirectory(entityx)
add_subdirectory(glad)
add_subdirectory(speex_resampler)
add_subdirectory(imgui)
add_subdirectory(imgui-filebrowser)

rigel_disable_warnings(entityx)
