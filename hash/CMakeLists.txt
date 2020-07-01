FILE(GLOB hash_source "*.c" "*.cpp" "*.h")

ADD_LIBRARY(hash ${hash_source})

# ADD_SUBDIRECTORY(tests)
