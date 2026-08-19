# set BOOST_INCLUDE_LIBRARIES to include specific libraries before including this script, e.g.
# set(BOOST_INCLUDE_LIBRARIES program_options threads)
set(Boost_NO_SYSTEM_PATHS TRUE)
set(Boost_USE_STATIC_LIBS    ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS     OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS   ON)  # only find release libs
set(Boost_USE_STATIC_RUNTIME ON) # Mac insists on ON for boost_program_options
#set(BOOST_USE_MULTITHREADED ON)
#set(Boost_DEBUG ON)
set(Boost_VERSION 1.86.0)

if (POLICY CMP0167)
  cmake_policy (SET CMP0167 NEW)
endif ()
if (BOOST_INCLUDE_LIBRARIES)
  find_package(Boost ${Boost_VERSION} COMPONENTS ${BOOST_INCLUDE_LIBRARIES} QUIET)
else(BOOST_INCLUDE_LIBRARIES)
  find_package(Boost ${Boost_VERSION} QUIET)
endif(BOOST_INCLUDE_LIBRARIES)

if (Boost_FOUND)
  message(STATUS "Found Boost: ${Boost_DIR}")
else(Boost_FOUND)
  message(STATUS "Failed to find Boost (${BOOST_INCLUDE_LIBRARIES}), going to compile from source")
  if (FIND_FATAL)
    message(FATAL_ERROR "Failed to find Boost with CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
  endif(FIND_FATAL)
  set(BOOST_ENABLE_CMAKE ON)
  FetchContent_Declare(
    Boost
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    #FIND_PACKAGE_ARGS NAMES Boost COMPONENTS ${BOOST_INCLUDE_LIBRARIES}
    URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
    URL_HASH SHA256=f48b48390380cfb94a629872346e3a81370dc498896f16019ade727ab72eb1ec
    # GIT_REPOSITORY https://github.com/boostorg/boost.git
    # GIT_TAG boost-${Boost_VERSION}
    # GIT_SHALLOW TRUE # get only the last commit version
    # GIT_PROGRESS TRUE # show progress of download
    USES_TERMINAL_DOWNLOAD TRUE # show progress in ninja generator
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON
    USES_TERMINAL_INSTALL ON
    )
  FetchContent_MakeAvailable(Boost)
  if (Boost_POPULATED)
    message(STATUS "Found populated Boost (${BOOST_INCLUDE_LIBRARIES}): ${boost_SOURCE_DIR}")
  else (Boost_POPULATED)
    FetchContent_MakeAvailable(Boost)
    add_subdirectory(${boost_SOURCE_DIR} ${boost_BINARY_DIR} EXCLUDE_FROM_ALL)
    # workaround for cmake complaint that boost is not among exports:
    install(TARGETS boost_headers boost_math boost_assert boost_concept_check boost_config boost_core
            boost_integer boost_lexical_cast boost_predef boost_random boost_static_assert boost_throw_exception
            boost_preprocessor boost_type_traits boost_array boost_container boost_numeric_conversion boost_range
            boost_dynamic_bitset boost_io boost_system boost_utility boost_intrusive boost_move boost_conversion
            boost_mpl boost_container_hash boost_detail boost_iterator boost_optional boost_regex boost_tuple
            boost_variant2 boost_winapi boost_smart_ptr boost_typeof boost_describe boost_mp11 boost_function_types
            boost_fusion boost_functional boost_function boost_bind
            EXPORT UUtilsConfig DESTINATION ${CMAKE_INSTALL_LIBDIR})
    message(STATUS "Got Boost (${BOOST_INCLUDE_LIBRARIES}): ${boost_SOURCE_DIR}")
  endif(Boost_POPULATED)
endif(Boost_FOUND)
