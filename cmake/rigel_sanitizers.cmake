# allow CTest to work with address sanitizer (requires cmake > 3.18)
# eg. ctest -V -T MemCheck
# https://gitlab.kitware.com/cmake/cmake/-/issues/20584
if(CMAKE_BUILD_TYPE STREQUAL "Asan")
  # https://cmake.org/cmake/help/latest/variable/CTEST_MEMORYCHECK_TYPE.html
  set(MEMORYCHECK_TYPE "AddressSanitizer" CACHE STRING "Use ASAN in MemCheck" FORCE)
endif()

# only enable sanitizers on clang and gcc via CMAKE_BUILD_TYPE
# flags for C/CXX are setup using `CMAKE_C_FLAGS_<CMAKE_BUILD_TYPE-in-all-caps>
# use CMAKE_BUILD_TYPE=Asan to enabled these flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # ASAN flags
  set(CMAKE_C_FLAGS_ASAN
    "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C compiler for Asan build type or configuration." FORCE)
  set(CMAKE_CXX_FLAGS_ASAN
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C++ compiler for Asan build type or configuration." FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_ASAN
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
    "Linker flags to be used to create executables for Asan build type." FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS_ASAN
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
    "Linker flags to be used to create shared libraries for Asan build type." FORCE)
endif()
