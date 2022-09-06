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

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)

install(DIRECTORY include DESTINATION .)
install(TARGETS UDBM EXPORT UDBMConfig LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT UDBMConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM )
install(FILES ${CMAKE_BINARY_DIR}/include/dbm/config.h DESTINATION include/dbm/)
