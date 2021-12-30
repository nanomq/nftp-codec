cmake_minimum_required(VERSION 3.13)

project(nftp-codec)

if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DDEBUG)
endif(DEBUG)

add_subdirectory(hashtable)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hashtable)

set(SOURCES 
  src/file.c
  src/hash.c
  src/iovs.c
  src/codec.c
  src/proto.c
  src/nftp.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(nftp-codec SHARED ${SOURCES})
add_library(nftp-codec-static STATIC ${SOURCES})

if(TEST)
  add_executable(test
	  test/test.c
	  test/hash.c
	  test/file.c
	  test/iovs.c
	  test/codec.c
	  test/proto.c)
  target_link_libraries(test nftp-codec hashtable)
endif(TEST)

if(NOT DEBUG)
  target_compile_options(nftp-codec PUBLIC -O3 -Os -g)
endif()
