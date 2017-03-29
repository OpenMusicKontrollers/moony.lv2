cmake_minimum_required(VERSION 2.8)

project(varchunk)

set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# find pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
	
include(CTest)
if(${BUILD_TESTING})
	add_executable(test_varchunk
		test_varchunk.c)
	target_link_libraries(test_varchunk ${CMAKE_THREAD_LIBS_INIT})
	
	add_test(API-Test test_varchunk)
endif()
