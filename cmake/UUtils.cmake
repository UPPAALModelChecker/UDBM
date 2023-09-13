find_package(UUtils 2.0.2 COMPONENTS base hash debug QUIET)

if (UUtils_FOUND)
  message(STATUS "Found UUtils: ${UUtils_DIR}")
else(UUtils_FOUND)
    message(STATUS "Failed to find UUtils, going to compile from source.")
    if (FIND_FATAL)
      message(FATAL_ERROR "Failed to find UUtils with CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    endif(FIND_FATAL)
    include(FetchContent)
    set(UUtils_WITH_TESTS OFF CACHE BOOL "UUtils tests")
    set(UUtils_WITH_BENCHMARKS OFF CACHE BOOL "UUtils benchmarks")
    FetchContent_Declare(
            UUtils
            #GIT_REPOSITORY https://github.com/mikucionisaau/UUtils.git
            #GIT_TAG cmake-alias
            GIT_REPOSITORY https://github.com/UPPAALModelChecker/UUtils.git
            GIT_TAG v2.0.3
            GIT_SHALLOW TRUE # get only the last commit version
            GIT_PROGRESS TRUE # show progress of download
            # FIND_PACKAGE_ARGS NAMES doctest
            USES_TERMINAL_DOWNLOAD TRUE # show progress in ninja generator
            USES_TERMINAL_CONFIGURE ON
            USES_TERMINAL_BUILD ON
            USES_TERMINAL_INSTALL ON
    )
    FetchContent_GetProperties(UUtils)
    if (uutils_POPULATED)
        message(STATUS "Found populated UUtils: ${uutils_SOURCE_DIR}")
    else (uutils_POPULATED)
        FetchContent_Populate(UUtils)
        add_subdirectory(${uutils_SOURCE_DIR} ${uutils_BINARY_DIR} EXCLUDE_FROM_ALL)
        message(STATUS "Got UUtils: ${uutils_SOURCE_DIR}")
    endif (uutils_POPULATED)
endif(UUtils_FOUND)
