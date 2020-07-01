FILE(GLOB base_source "*.c" "*.cpp" "*.h")

ADD_LIBRARY(base ${base_source})

ADD_SUBDIRECTORY(tests)
