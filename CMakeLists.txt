cmake_minimum_required(VERSION 3.16)
project(UDBM VERSION 2.0.11 LANGUAGES CXX C)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(UDBM_WITH_TESTS "UDBM Unit tests" OFF)
option(UDBM_STATIC "Static linking" OFF)
option(SSP "Stack-Smashing Protector" OFF)
option(UBSAN "Undefined Behavior Sanitizer" OFF)
option(ASAN "Address Sanitizer" OFF)

cmake_policy(SET CMP0048 NEW) # project() command manages VERSION variables
set(CMAKE_CXX_STANDARD 17)
set(UDBM_PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(UDBM_VERSION "${PACKAGE_VERSION}")
set(ENABLE_STORE_MINGRAPH 1)
CONFIGURE_FILE("src/config.h.cmake" "include/dbm/config.h")

include(cmake/clang-tidy.cmake)
include(cmake/doctest.cmake)

find_package(xxHash 0.8.0 CONFIG REQUIRED)
find_package(UUtils 1.1.1 REQUIRED COMPONENTS base hash debug)

if (SSP)
    message(STATUS "Enabled Stack Smashing Protector")
    add_compile_options(-fstack-protector)
    add_link_options(-fstack-protector)
endif(SSP)

if (UBSAN OR ASAN)
    add_compile_options(-fno-omit-frame-pointer)
    add_link_options(-fno-omit-frame-pointer)
endif()
if (UBSAN)
    message(STATUS "Enabled Undefined Behavior Sanitizer")
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif(UBSAN)
if (ASAN)
    message(STATUS "Enabled Address Sanitizer")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif(ASAN)

if(UDBM_STATIC)
	set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif(UDBM_STATIC)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wpedantic -Wall -Wextra -Werror=vla -Wno-unused-parameter)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory("src")

if(UDBM_WITH_TESTS)
    enable_testing()
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
