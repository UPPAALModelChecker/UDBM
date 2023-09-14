#find_package(xxHash 0.8.0 CONFIG QUIET)
set(XXHASH_VERSION_MINIMUM 0.8.0)
find_file(XXHASH_PATH xxhash.h)
if (XXHASH_PATH)
  cmake_path(GET XXHASH_PATH PARENT_PATH xxHash_DIR)
  file(READ "${XXHASH_PATH}" XXHASH_H)
  string(REGEX MATCH "#define XXH_VERSION_MAJOR([ \t])*([0-9])*" _ ${XXHASH_H})
  set(XXHASH_VERSION_MAJOR ${CMAKE_MATCH_2})
  string(REGEX MATCH "#define XXH_VERSION_MINOR([ \t])*([0-9])*" _ ${XXHASH_H})
  set(XXHASH_VERSION_MINOR ${CMAKE_MATCH_2})
  string(REGEX MATCH "#define XXH_VERSION_RELEASE([ \t])*([0-9])*" _ ${XXHASH_H})
  set(XXHASH_VERSION_PATCH ${CMAKE_MATCH_2})
  set(XXHASH_VERSION "${XXHASH_VERSION_MAJOR}.${XXHASH_VERSION_MINOR}.${XXHASH_VERSION_PATCH}")
  if (XXHASH_VERSION VERSION_GREATER_EQUAL ${XXHASH_VERSION_MINIMUM})
    set(xxHash_FOUND TRUE)
    cmake_path(GET XXHASH_PATH PARENT_PATH XXHASH_INCLUDE_DIR)
    add_library(xxHash INTERFACE)
    target_compile_definitions(xxHash INTERFACE XXH_INLINE_ALL)
    target_include_directories(xxHash
            INTERFACE
            $<BUILD_INTERFACE:${XXHASH_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
  else()
    message(STATUS "Found xxHash-${XXHASH_VERSION} is too old, need at lease ${XXHASH_VERSION_MINIMUM}")
  endif()
endif(XXHASH_PATH)

if (xxHash_FOUND)
  message(STATUS "Found xxHash-${XXHASH_VERSION}: ${XXHASH_PATH}")
else(xxHash_FOUND)
  message(STATUS "Failed to find xxHash, going to compile from source.")
  if (FIND_FATAL)
    message(FATAL_ERROR "Failed to find xxHash with CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
  endif(FIND_FATAL)
  include(ExternalProject)
  set(XXHASH_BUILD_ENABLE_INLINE_API ON CACHE BOOL "adds xxhash.c for the -DXXH_INLINE_ALL api. Default ON")
  set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "build the command line binary. Default ON")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "build dynamic library. Default ON")
  # set(DISPATCH OFF CACHE BOOL "enable dispatch mode. Default OFF")
  FetchContent_Declare(
    xxHash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash
    GIT_TAG v0.8.2
    GIT_SHALLOW TRUE # get only the last commit version
    GIT_PROGRESS TRUE # show progress of download
    SOURCE_SUBDIR cmake_unofficial # CMakeLists.txt is not in the main folder
    FIND_PACKAGE_ARGS NAMES xxHash COMPONENTS xxhash # for future when xxhash supports cmake
    USES_TERMINAL_DOWNLOAD TRUE # show progress in ninja generator
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON
    USES_TERMINAL_INSTALL ON
    )
  FetchContent_GetProperties(xxHash)
  if (xxhash_POPULATED)
    message(STATUS "Found populated xxHash: ${xxhash_SOURCE_DIR}")
  else (xxhash_POPULATED)
    FetchContent_Populate(xxHash)
    add_subdirectory(${xxhash_SOURCE_DIR}/cmake_unofficial ${xxhash_BINARY_DIR} EXCLUDE_FROM_ALL)
    add_library(xxHash INTERFACE)
    target_compile_definitions(xxHash INTERFACE XXH_INLINE_ALL)
    target_include_directories(xxHash
            INTERFACE
            $<BUILD_INTERFACE:${xxhash_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    message(STATUS "Got xxHash: ${xxhash_SOURCE_DIR}")
  endif (xxhash_POPULATED)
endif(xxHash_FOUND)
