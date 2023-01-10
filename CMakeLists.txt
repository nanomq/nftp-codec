cmake_minimum_required(VERSION 3.13)

project(nftp-codec)

set(DEBUG ON)
set(TEST ON)

if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
  add_definitions(-DDEBUG)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif(DEBUG)

#add_subdirectory(hashtable)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hashtable)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES 
  src/hash.c
  src/file.c
  src/vector.c
  src/iovs.c
  src/iter.c
  src/codec.c
  src/proto.c
  hashtable/hashtable.c
)

add_library(nftp-codec SHARED ${SOURCES})
add_library(nftp-codec-static STATIC ${SOURCES})

if(TEST)
  add_executable(test
	  test/test.h
	  test/test.c
	  test/hash.c
	  test/file.c
	  test/vector.c
	  test/iovs.c
	  test/iter.c
	  test/codec.c
	  test/proto.c)
  target_link_libraries(test nftp-codec)
endif(TEST)

if(NOT DEBUG)
  target_compile_options(nftp-codec PUBLIC -O3 -Os -g)
endif()
