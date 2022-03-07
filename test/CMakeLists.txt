# dbm tests...

set(libs UDBM UUtils::udebug UUtils::base UUtils::hash)

if (TESTING)
  file(GLOB test_sources "test_*.c" "test_*.cpp")

  foreach(source ${test_sources})
    get_filename_component(test_target ${source} NAME_WE)
    add_executable(${test_target} ${source})
    target_link_libraries(${test_target} ${libs})
  endforeach()

  # comments contain expected time of Linux debug build, YMMV, whereas timeouts are worst case
  add_test(NAME dbm_dbm_1_10 COMMAND test_dbm 1 10)
  set_tests_properties(dbm_dbm_1_10 PROPERTIES TIMEOUT 80) # 68s
  add_test(NAME dbm_ext_10 COMMAND test_ext 10)
  add_test(NAME dbm_extrapolation_10 COMMAND test_extrapolation 10)
  add_test(NAME dbm_fed_1_7_7    COMMAND test_fed 1 7 7) # failing test on L64
  set_tests_properties(dbm_fed_1_7_7 PROPERTIES TIMEOUT 2700) # 41min on Linux32!
  add_test(NAME dbm_fed_dbm_1_10 COMMAND test_ext 1 10)
  add_test(NAME dbm_mingraph_1_10   COMMAND test_mingraph 1 10) # failing test on L64-debug
  set_tests_properties(dbm_mingraph_1_10 PROPERTIES TIMEOUT 45) # 36s on Win64
  add_test(NAME dbm_mingraph_1_10_1 COMMAND test_mingraph 1 10 1) # failing test on L64-debug
  set_tests_properties(dbm_mingraph_1_10_1 PROPERTIES TIMEOUT 45) # 39s on Win64
  add_test(NAME dbm_valuation_20    COMMAND test_valuation 20)
  add_test(NAME test_allocation COMMAND test_allocation)

  set_tests_properties(dbm_dbm_1_10 dbm_fed_1_7_7 PROPERTIES TIMEOUT 1200)
endif()
