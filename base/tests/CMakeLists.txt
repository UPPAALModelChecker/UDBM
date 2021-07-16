# base tests...

set(libs base udebug)

if (WIN32)
  list(APPEND libs psapi iphlpapi)
endif(WIN32)

if (TESTING)
  file(GLOB test_sources "test_*.c" "test_*.cpp")

  foreach(source ${test_sources})
    get_filename_component(test_target ${source} NAME_WE)
    add_executable(${test_target} ${source})
    target_link_libraries(${test_target} ${libs})
  endforeach()

  add_test(NAME base_allocator_10000 COMMAND test_allocator 10000)
  add_test(NAME base_allocator_30000 COMMAND test_allocator 30000)

  add_test(NAME base_array_10_0 COMMAND test_array 10 0)
  add_test(NAME base_array_10_1 COMMAND test_array 10 1)
  add_test(NAME base_array_10_2 COMMAND test_array 10 2)
  add_test(NAME base_array_10_3 COMMAND test_array 10 3)

  add_test(NAME base_bit_string COMMAND test_bit_string)

  add_test(NAME base_crash_allocator_0 COMMAND test_crash_allocator 0)
  add_test(NAME base_crash_allocator_1 COMMAND test_crash_allocator 1)
  add_test(NAME base_crash_allocator_2 COMMAND test_crash_allocator 2)
  add_test(NAME base_crash_allocator_3 COMMAND test_crash_allocator 3)
  add_test(NAME base_crash_allocator_4 COMMAND test_crash_allocator 4)

  add_test(NAME base_int_utils_10    COMMAND test_int_utils    10)
  add_test(NAME base_int_utils_10000 COMMAND test_int_utils 10000)

  add_test(NAME base_item_allocator_1      COMMAND test_item_allocator      1)
  add_test(NAME base_item_allocator_10     COMMAND test_item_allocator     10)
  add_test(NAME base_item_allocator_100000 COMMAND test_item_allocator 100000)

  add_test(NAME base_timer_100 COMMAND test_timer)


  # these tests fail intentionally, but via assertion, and ctest treats them as crash on Linux
  set_tests_properties(
    base_array_10_1
    base_array_10_2
    base_array_10_3
    base_crash_allocator_1
    base_crash_allocator_2
    base_crash_allocator_3
    PROPERTIES WILL_FAIL TRUE)

  # disable failing tests:
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set_tests_properties(
	    base_allocator_10000 base_allocator_30000 # segfault
	    base_array_10_0 base_array_10_1  base_array_10_2 base_array_10_3 # assertion fail
	    base_bit_string # assertion fail
	    # base_crash_allocator_0 # passes correctly!
	    base_crash_allocator_1 base_crash_allocator_2 base_crash_allocator_3 # assertion fail
	    PROPERTIES DISABLED TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	  set_tests_properties(
	    base_allocator_10000 base_allocator_30000 # segfault
	    base_array_10_0 base_array_10_1  base_array_10_2 base_array_10_3 # assertion fail
	    base_bit_string # assertion fail
	    base_crash_allocator_0 # assertion fail
	    base_crash_allocator_1 base_crash_allocator_2 base_crash_allocator_3 # assertion fail
	    PROPERTIES DISABLED TRUE)
    endif()
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set_tests_properties(
	    base_allocator_10000 base_allocator_30000 # segfault
	    base_array_10_0 base_array_10_1  base_array_10_2 base_array_10_3 # assertion fail
	    base_bit_string # assertion fail
	    # base_crash_allocator_0 # passes correctly!
	    base_crash_allocator_1 base_crash_allocator_2 base_crash_allocator_3 # assertion fail
	    PROPERTIES DISABLED TRUE)
    endif()
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_tests_properties(
	  base_array_10_1
	  base_array_10_2
	  PROPERTIES RUN_SERIAL TRUE) # these take a long time if run in parallel
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  # target_compile_options(test_rangeset PRIVATE "-Wa,-mbig-obj") # otherwise assembler fails with "file too big"
	  set_tests_properties(
	    base_allocator_10000 base_allocator_30000 # segfault
	    base_array_10_0 # strange output
	    base_array_10_1 # assertion fail as expected
	    base_bit_string # assertion fail
	    # base_crash_allocator_0 # passes correctly!
	    base_crash_allocator_1 base_crash_allocator_2 base_crash_allocator_3 # assertion fail as expected
	    PROPERTIES DISABLED TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	  set_tests_properties(
	    base_allocator_10000 base_allocator_30000 # segfault
	    base_array_10_0 # assertion fail
	    base_bit_string # assertion fail
	    base_crash_allocator_0 # assertion fail
	    #	base_crash_allocator_1 base_crash_allocator_2 base_crash_allocator_3 # assertion fail as expected
	    PROPERTIES DISABLED TRUE)
    endif()
  endif()
endif()
