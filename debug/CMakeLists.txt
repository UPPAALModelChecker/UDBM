file(GLOB udebug_source "*.c" "*.cpp" "*.h")

add_library(udebug ${udebug_source} )

add_subdirectory(tests)
