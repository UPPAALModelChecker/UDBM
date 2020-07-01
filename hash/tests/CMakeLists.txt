# hash tests...

SET(libs hash base udebug)

IF(WIN32)
  MESSAGE("Tests for hash module will not be built for Windows...")
ELSE(WIN32)
  GENERATE_C_CPP_TESTS("hash" libs)
ENDIF(WIN32)
