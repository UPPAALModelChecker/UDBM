add_library(UDBM STATIC DBMAllocator.cpp dbm.c fed_dbm.cpp mingraph.c mingraph_read.c partition.cpp print.cpp gen.c
        mingraph_cache.cpp mingraph_relation.c pfed.cpp fed.cpp infimum.cpp mingraph_equal.c mingraph_write.c
        priced.cpp valuation.cpp)
set_property(TARGET UDBM PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET UDBM PROPERTY VISIBILITY_INLINES_HIDDEN ON)
if (NOT CMAKE_SYSTEM_NAME STREQUAL Windows) # unknown argument: '-fno-keep-inline-dllexport'
    set_target_properties(UDBM PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()
target_link_libraries(UDBM PRIVATE UUtils::base UUtils::udebug UUtils::hash)

target_include_directories(UDBM
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include> # dbm/config.h
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
