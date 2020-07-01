FILE(GLOB udebug_source "*.c" "*.cpp" "*.h")

ADD_LIBRARY(udebug ${udebug_source})

ADD_SUBDIRECTORY(tests)
