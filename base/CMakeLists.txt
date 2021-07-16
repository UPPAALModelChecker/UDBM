file(GLOB base_source "*.c" "*.cpp")
add_library(base ${base_source})

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_link_libraries(base udebug iphlpapi psapi)
else()
  target_link_libraries(base udebug)
endif()

add_subdirectory(tests)
