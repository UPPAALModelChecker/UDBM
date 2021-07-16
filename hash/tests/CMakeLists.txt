# hash tests...

set(libs hash base udebug)

if (TESTING)
  file(GLOB test_sources "test_*.c" "test_*.cpp")

  foreach(source ${test_sources})
    get_filename_component(test_target ${source} NAME_WE)
    add_executable(${test_target} ${source})
    target_link_libraries(${test_target} ${libs})
  endforeach()

  add_test(NAME hash_compute COMMAND test_compute)
  add_test(NAME hash_tables_0 COMMAND test_tables 0)
  add_test(NAME hash_tables_1 COMMAND test_tables 1)
  add_test(NAME hash_tables_10000 COMMAND test_tables 10000)
  add_test(NAME hash_tables_1000000 COMMAND test_tables 1000000)

  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_tests_properties(
	  hash_tables_1000000
	  PROPERTIES RUN_SERIAL TRUE)
  endif()
endif()
