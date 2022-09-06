add_library(UDBM DBMAllocator.cpp dbm.c fed_dbm.cpp mingraph.c mingraph_read.c partition.cpp print.cpp gen.c
        mingraph_cache.cpp mingraph_relation.c pfed.cpp fed.cpp infimum.cpp mingraph_equal.c mingraph_write.c
        priced.cpp)
target_link_libraries(UDBM UUtils::base UUtils::udebug UUtils::hash)

target_include_directories(UDBM
    PRIVATE
        # where the library itself will look for its internal headers
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
