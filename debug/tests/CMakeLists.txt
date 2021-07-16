# debug tests...

set(libs base udebug)

if (TESTING)
  file(GLOB test_sources "test_*.c" "test_*.cpp")

  foreach(source ${test_sources})
    get_filename_component(test_target ${source} NAME_WE)
    add_executable(${test_target} ${source})
    target_link_libraries(${test_target} ${libs})
  endforeach()

  add_test(NAME debug_new_0 COMMAND test_new 0)
  add_test(NAME debug_new_1 COMMAND test_new 1)
  add_test(NAME debug_new_2 COMMAND test_new 2) # failing test: L64-debug
  add_test(NAME debug_new_3 COMMAND test_new 3)

  add_test(NAME debug_utils_10 COMMAND test_utils 10)  # failing test: L64-debug
  add_test(NAME debug_utils_20 COMMAND test_utils 20)  # failing test: L64-debug
  add_test(NAME debug_utils_1  COMMAND test_utils  1)  # failing test: L64-debug
  add_test(NAME debug_utils_7  COMMAND test_utils  7)  # failing test: L64-debug
  add_test(NAME debug_utils_50 COMMAND test_utils 50)  # failing test: L64-debug

  # mark tests which are supposed to fail
  set_tests_properties(
    debug_new_2
    debug_utils_10
    debug_utils_20
    debug_utils_1
    debug_utils_7
    debug_utils_50
    PROPERTIES WILL_FAIL TRUE)

  # disable failing tests:
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set_tests_properties(
	    debug_new_2
	    debug_utils_10
	    debug_utils_20
	    debug_utils_1
	    debug_utils_7
	    debug_utils_50
	    PROPERTIES DISABLED TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	  set_tests_properties(
	    debug_new_2
	    debug_utils_10
	    debug_utils_20
	    debug_utils_1
	    debug_utils_7
	    debug_utils_50
	    PROPERTIES DISABLED TRUE)
    endif()
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set_tests_properties(
	    debug_new_2
	    debug_utils_10
	    debug_utils_20
	    debug_utils_1
	    debug_utils_7
	    debug_utils_50
	    PROPERTIES DISABLED TRUE)
    endif()
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_tests_properties(
	  debug_new_3
	  PROPERTIES RUN_SERIAL TRUE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set_tests_properties(
	    debug_new_2
	    debug_utils_10
	    debug_utils_20
	    debug_utils_1
	    debug_utils_7
	    debug_utils_50
	    PROPERTIES DISABLED TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	  set_tests_properties(
	    debug_new_2
	    debug_utils_10
	    debug_utils_20
	    debug_utils_1
	    debug_utils_7
	    debug_utils_50
	    PROPERTIES DISABLED TRUE)
    endif()
  endif()
endif()
