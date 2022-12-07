# dbm tests...

set(libs UDBM UUtils::udebug UUtils::base UUtils::hash)

include(../cmake/doctest.cmake)

add_library(doctest_main OBJECT doctest_main.cpp)
target_link_libraries(doctest_main PRIVATE doctest::doctest)

file(GLOB test_c_sources "test_*.c" "test_allocation.cpp")
foreach(source ${test_c_sources})
  get_filename_component(test_target ${source} NAME_WE)
  add_executable(${test_target} ${source})
  target_link_libraries(${test_target} ${libs})
endforeach()

file(GLOB test_cpp_sources test_fed.cpp test_fed_dbm.cpp test_fp_intersection.cpp test_valuation.cpp test_constraint.cpp)
foreach(source ${test_cpp_sources})
  get_filename_component(test_target ${source} NAME_WE)
  add_executable(${test_target} ${source})
  target_link_libraries(${test_target} ${libs} doctest_main)
endforeach()

# comments contain expected time of Linux debug build, YMMV, whereas timeouts
# are worst case
add_test(NAME test_dbm_1_10 COMMAND test_dbm 1 10)
set_tests_properties(test_dbm_1_10 PROPERTIES TIMEOUT 80) # 68s
add_test(NAME test_ext_10 COMMAND test_ext 10)
add_test(NAME test_extrapolation_10 COMMAND test_extrapolation 10)
add_test(NAME test_fed COMMAND test_fed) # failing test on L64
set_tests_properties(test_fed PROPERTIES TIMEOUT 2700) # 41min on Linux32!
add_test(NAME test_fed_dbm COMMAND test_fed_dbm)
add_test(NAME test_mingraph_1_10 COMMAND test_mingraph 1 10) # failing test on
                                                            # L64-debug
set_tests_properties(test_mingraph_1_10 PROPERTIES TIMEOUT 45) # 36s on Win64
add_test(NAME test_mingraph_1_10_1 COMMAND test_mingraph 1 10 1) # failing test
                                                                # on L64-debug
set_tests_properties(test_mingraph_1_10_1 PROPERTIES TIMEOUT 45) # 39s on Win64
add_test(NAME test_valuation COMMAND test_valuation)
add_test(NAME test_allocation COMMAND test_allocation)
add_test(NAME test_constraint COMMAND test_constraint)

set_tests_properties(test_dbm_1_10 test_fed PROPERTIES TIMEOUT 1200)
