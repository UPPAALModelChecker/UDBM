find_package(doctest 2.4.11 QUIET)

if (doctest_FOUND)
  if (TARGET doctest::doctest_with_main)
    message(STATUS "Found doctest::doctest_with_main")
    add_library(doctest_with_main INTERFACE)
    target_link_libraries(doctest_with_main PRIVATE doctest::doctest_with_main)
  else(TARGET doctest::doctest_with_main) # workaround for old doctest
    message(STATUS "Workaround for doctest::doctest_with_main")
    add_library(doctest_with_main INTERFACE)
    target_compile_definitions(doctest_with_main INTERFACE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
    target_link_libraries(doctest_with_main INTERFACE doctest::doctest)
  endif(TARGET doctest::doctest_with_main)
  message(STATUS "Found doctest: ${doctest_DIR}")
else(doctest_FOUND)
    message(STATUS "Failed to find doctest, going to compile from source.")
    if (FIND_FATAL)
      message(FATAL_ERROR "Failed to find doctest with CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    endif(FIND_FATAL)
    include(FetchContent)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "doctest tests and examples")
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB ON CACHE BOOL "static lib (cmake target) with a default main entry point")
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "Skip the installation process")
    set(DOCTEST_USE_STD_HEADERS OFF CACHE BOOL "Use std headers")
    FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/doctest/doctest
            GIT_TAG v2.4.11
            GIT_SHALLOW TRUE # get only the last commit version
            GIT_PROGRESS TRUE # show progress of download
            # FIND_PACKAGE_ARGS NAMES doctest
            USES_TERMINAL_DOWNLOAD TRUE # show progress in ninja generator
            USES_TERMINAL_CONFIGURE ON
            USES_TERMINAL_BUILD ON
            USES_TERMINAL_INSTALL ON
    )
    FetchContent_GetProperties(doctest)
    if (doctest_POPULATED)
        message(STATUS "Found populated doctest: ${doctest_SOURCE_DIR}")
    else (doctest_POPULATED)
        FetchContent_Populate(doctest)
        add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR} EXCLUDE_FROM_ALL)
        message(STATUS "Got doctest: ${doctest_SOURCE_DIR}")
    endif (doctest_POPULATED)
endif(doctest_FOUND)
