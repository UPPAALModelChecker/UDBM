CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

cmake_policy(SET CMP0048 NEW) # project() command manages VERSION variables
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(PROFILE_COUNTERS OFF)
option(TESTING OFF)

# If ccache is installed, use it as compiler launcher
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache")
endif()

set(PACKAGE_NAME "UDBM")
set(VERSION 2.0.10)
set(PACKAGE_VERSION "${VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
project(UDBM VERSION ${VERSION} LANGUAGES CXX C)

# Use the faster gold linker if applicable
if (UNIX AND NOT APPLE)
    #  execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE ld_version)
    if ("${ld_version}" MATCHES "GNU gold")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
    endif()
endif()

message(STATUS "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

if (NOT DEFINED DEPEND_PREFIX)
  set(DEPEND_PREFIX ${CMAKE_PREFIX_PATH})
endif()
message(STATUS "DEPEND_PREFIX: ${DEPEND_PREFIX}")

set(BUILD_PREFIX "${CMAKE_BINARY_DIR}")
set(OUTPUT_ROOT "${BUILD_PREFIX}/udbm")
set(OUTPUT_DIR "${BUILD_PREFIX}/udbm")

set(MODULES_DIR "${CMAKE_SOURCE_DIR}")

set(EXTERNAL_INCLUDE "${DEPEND_PREFIX}/include")
set(INTERNAL_INCLUDE "${MODULES_DIR}/include")
set(EXTERNAL_LIBS ${DEPEND_PREFIX}/lib)
set(INTERNAL_LIBS ${OUTPUT_DIR}/lib ${OUTPUT_DIR})


if(APPLE)
	link_libraries(c)
endif(APPLE)

set(INCLUDE_DIRS ${INTERNAL_INCLUDE}
  ${EXTERNAL_INCLUDE}
  ${C_INCLUDE_PATH}
  ${CPLUS_INCLUDE_PATH})
include_directories(${INCLUDE_DIRS})
link_directories(${EXTERNAL_LIBS} ${INTERNAL_LIBS})

string(CONCAT UDBM_OPTIONS	      
    #"-DENABLE_STORE_MINGRAPH "			# storing the minimal graph in dbm_t (another kind of cache)
    "-DISABLE_ASSERTX"              # Disable exspensive asserts
)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

EXECUTE_PROCESS(COMMAND "git" "rev-parse" "HEAD"
                COMMAND "cut" "-c1-16"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE PACKAGE_REVISION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
string(TOUPPER "${PACKAGE_REVISION}" PACKAGE_REVISION)
set(PACKAGE_REVISION "rev. ${PACKAGE_REVISION}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}/bin)

if (NOT CMAKE_SIZEOF_VOID_P)
    message(FATAL_ERROR "Unknown architecture: the pointer size is undefined, use toolchain-file to define")
endif()
math(EXPR ARCHBITS "8*${CMAKE_SIZEOF_VOID_P}")
message(STATUS "Targeting CXX platform: ${CMAKE_CXX_PLATFORM_ID} ${ARCHBITS}-bits ${CMAKE}")
message(STATUS "Targeting C platform: ${CMAKE_C_PLATFORM_ID} ${ARCHBITS}-bits")
message(STATUS "With CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "With CC compiler: ${CMAKE_C_COMPILER}")
message(STATUS "UDBM options: ${UDBM_OPTIONS}")

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # C/C++ common flags:
    string(CONCAT C_CXX_FLAGS "-Wfatal-errors -Wall -Wextra -Wcast-align "
            "-Wduplicated-cond -Wduplicated-branches "
            "-Wlogical-op -Wmisleading-indentation -Wempty-body -fdelete-null-pointer-checks "
            "-Wformat=2 -Wno-unused-parameter")
    if (PROFILE_COUNTERS)
      message(STATUS "Enabling gprof code profiling")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
      set(CMAKE_EXE_LINKER_FLAGS "-pg ${CMAKE_EXE_LINKER_FLAGS}")
      set(CMAKE_SHARED_LINKER_FLAGS "-pg ${CMAKE_SHARED_LINKER_FLAGS}")
    endif()
    set(C_FLAGS ${C_FLAGS} ${C_CXX_FLAGS})
    set(CXX_FLAGS ${CXX_FLAGS} ${C_CXX_FLAGS})
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${UDBM_OPTIONS} ${CXX_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${UDBM_OPTIONS} ${C_FLAGS}")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${UDBM_OPTIONS} ${CXX_FLAGS}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} ${UDBM_OPTIONS} ${C_FLAGS}")
endif()

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
set(COMPILER_STRING "Compiled using ${UDBM_OPTIONS}") # pass the compiler options string to UCDD

set(MODULES base debug dbm hash)

if (TESTING)
  enable_testing()
endif()

foreach(MODULE ${MODULES})
  add_subdirectory("${MODULES_DIR}/${MODULE}")
endforeach()

message(STATUS "Build configuration:
         Default build mode:  ${CMAKE_BUILD_TYPE}
         Project Root:        ${PROJECT_ROOT}
         Project source:      ${MODULES_DIR}
         Prefix:              ${BUILD_PREFIX}
         Build location:      ${OUTPUT_DIR}")

ADD_CUSTOM_TARGET(debug
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
  # WORKING_DIRECTORY ${DEBUG_BUILD_DIR}
  COMMENT "Compile in debug mode"
)

ADD_CUSTOM_TARGET(release
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
  # WORKING_DIRECTORY ${RELEASE_BUILD_DIR}
  COMMENT "Compile in release mode"
)

INCLUDE (CheckIncludeFile)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("sstream" HAVE_SSTREAM)

EXECUTE_PROCESS(COMMAND "date" "+%B"
  OUTPUT_VARIABLE PACKAGE_MONTH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
EXECUTE_PROCESS(COMMAND "date" "+%Y"
  OUTPUT_VARIABLE PACKAGE_YEAR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

SET(PACKAGE_BUGREPORT "uppaal@cs.aau.dk")
SET(PACKAGE_TARNAME "UDBM")
SET(PACKAGE_URL "https://www.uppaal.org/")



CONFIGURE_FILE("${MODULES_DIR}/config.h.cmake"
	       "${MODULES_DIR}/include/config.h"
)

