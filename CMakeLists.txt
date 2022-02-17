cmake_minimum_required(VERSION 3.16)
project(UDBM VERSION 2.0.8 LANGUAGES CXX C)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(TESTING OFF)
option(STATIC OFF)

cmake_policy(SET CMP0048 NEW) # project() command manages VERSION variables
set(CMAKE_CXX_STANDARD 17)
set(UDBM_PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(UDBM_VERSION "${PACKAGE_VERSION}")
set(ENABLE_STORE_MINGRAPH 1)
CONFIGURE_FILE("src/config.h.cmake" "include/dbm/config.h")


find_package(UUtils 1.1.0 REQUIRED COMPONENTS base hash debug PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/UUtils)

# Bit of a hack. UUtils requires xxhash, but can fetch it like this.
# But it can also find it in the system. If UDBM can find it in the system
# we assume that UUtils found it in the system, and all is linked properly
# If not, we also need to include it, such that we can find it when it's included
# in include/hash/compute.h
find_library(xxhash NAMES xxhash xxHash)
if(NOT xxhash)
    message("XXHash not found. Downloading via fetchContent")
    include(FetchContent)
    FetchContent_Declare(xxhashc
        GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
        GIT_TAG 35b0373c697b5f160d3db26b1cbb45a0d5ba788c
        )
    FetchContent_MakeAvailable(xxhashc)
    include_directories("${xxhashc_SOURCE_DIR}")
endif()

add_library(UDBM src/DBMAllocator.cpp src/dbm.c src/fed_dbm.cpp src/mingraph.c src/mingraph_read.c src/partition.cpp src/print.cpp src/gen.c src/mingraph_cache.cpp src/mingraph_relation.c src/pfed.cpp src/fed.cpp src/infimum.cpp src/mingraph_equal.c src/mingraph_write.c src/priced.cpp)
target_link_libraries(UDBM UUtils::base UUtils::udebug UUtils::hash)

if(STATIC)
	set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif(STATIC)

target_include_directories(UDBM
    PRIVATE
        # where the library itself will look for its internal headers
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if(TESTING)
    enable_testing()
    add_subdirectory("test")
endif(TESTING)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)


install(DIRECTORY include DESTINATION .)
install(TARGETS UDBM EXPORT UDBMConfig LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT UDBMConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/dbm/config.h DESTINATION include/dbm/)
