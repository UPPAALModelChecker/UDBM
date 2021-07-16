set(DBM_VERSION "2.0.8" PARENT_SCOPE)
file(GLOB dbm_source "*.c" "*.cpp" "*.h")

add_library(dbm DBMAllocator.cpp  
	dbm.c
	fed_dbm.cpp  
	mingraph.c          mingraph_read.c      partition.cpp  print.cpp
gen.c        mingraph_cache.cpp  mingraph_relation.c  pfed.cpp
fed.cpp           infimum.cpp  mingraph_equal.c    mingraph_write.c     priced.cpp 
)
target_link_libraries(dbm base udebug hash)

add_subdirectory(tests)
