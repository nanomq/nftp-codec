cmake_minimum_required(VERSION 3.13)

project(nftp-codec)

option(DEBUG "This is a default option for DEBUG" OFF)
option(TEST "This is a default option for EXAMPLE" OFF)

if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -fsanitize=address")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -fsanitize=address -fsanitize=leak")
  endif()
  add_definitions(-DDEBUG)
endif(DEBUG)

set(SOURCES 
  src/file.c
  src/hash.c
  src/iovs.c
  src/nftp.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(nftp-codec ${SOURCES})

if(TEST)
  add_executable(test-hash test/hash.c )
  target_link_libraries(test-hash nftp-codec)
  add_executable(test-file test/file.c)
  target_link_libraries(test-file nftp-codec)
endif(TEST)

if(NOT DEBUG)
  target_compile_options(nftp-codec PUBLIC -O3 -Os -g)
endif()
