# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-4
  VERSION 0.1
  LANGUAGES FORTRAN)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-4 main.90)
target_link_libraries(dlite-example-4 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-4 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
