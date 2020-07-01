# io tests...

SET(libs io base udebug)

IF(WIN32)
  LIST(APPEND libs psapi ws2_32 iphlpapi)
ENDIF(WIN32)

GENERATE_C_CPP_TESTS("io" libs)
