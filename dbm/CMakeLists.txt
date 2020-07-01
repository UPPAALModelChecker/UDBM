FILE(GLOB dbm_source "*.c" "*.cpp" "*.h")

ADD_LIBRARY(dbm ${dbm_source})

# ADD_SUBDIRECTORY(tests)
