FILE(GLOB io_source "*.c" "*.cpp" "*.h")

ADD_LIBRARY(io ${io_source})

# ADD_SUBDIRECTORY(tests)
