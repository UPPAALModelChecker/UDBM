find_package(doctest 2.4.9 QUIET)
if(doctest_FOUND)
  message(STATUS "Found doctest preinstalled.")
else(doctest_FOUND)
  message(STATUS "Failed to find doctest, going to make it from scratch.")
  set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "doctest tests")
  include(FetchContent)
  FetchContent_Declare(doctest
    GIT_REPOSITORY git@github.com:doctest/doctest.git
    GIT_TAG v2.4.9
    GIT_SHALLOW ON
  )
  FetchContent_MakeAvailable(doctest)
endif(doctest_FOUND)
