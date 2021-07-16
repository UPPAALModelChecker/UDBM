file(GLOB hash_source "*.c" "*.cpp" "*.h")

add_library(hash ${hash_source})

add_subdirectory(tests)
