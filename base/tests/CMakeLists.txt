# base tests...

SET(libs base udebug)

IF(WIN32)
  LIST(APPEND libs psapi iphlpapi)
ENDIF(WIN32)

GENERATE_C_CPP_TESTS("base" libs)

