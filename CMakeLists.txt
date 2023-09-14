cmake_minimum_required(VERSION 3.16)
project(UDBM VERSION 2.0.11 LANGUAGES CXX C)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(UDBM_WITH_TESTS "UDBM Unit tests" ON)
option(UDBM_STATIC "Static linking" OFF)
option(FIND_FATAL "Stop upon find_package errors" OFF)

cmake_policy(SET CMP0048 NEW) # project() command manages VERSION variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(UDBM_PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(UDBM_VERSION "${PACKAGE_VERSION}")
set(ENABLE_STORE_MINGRAPH 1)
CONFIGURE_FILE("src/config.h.cmake" "include/dbm/config.h")

include(cmake/sanitizer.cmake)
include(cmake/clang-tidy.cmake)
if (UDBM_WITH_TESTS)
    include(cmake/doctest.cmake)
endif (UDBM_WITH_TESTS)
include(cmake/UUtils.cmake)

if(UDBM_STATIC)
	set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif(UDBM_STATIC)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wpedantic -Wall -Wextra -Werror=vla -Wno-unused-parameter)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
endif()

if(UDBM_WITH_TESTS)
    enable_testing()
endif(UDBM_WITH_TESTS)

add_subdirectory("src")

if(UDBM_WITH_TESTS)
    add_subdirectory(test)
endif(UDBM_WITH_TESTS)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)
install(DIRECTORY include DESTINATION .)
install(TARGETS UDBM EXPORT UDBMConfig
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT UDBMConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/UDBMConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UDBM )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/dbm/config.h DESTINATION include/dbm/)
