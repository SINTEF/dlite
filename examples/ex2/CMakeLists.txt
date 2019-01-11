# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-2
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(Finddlite)
find_package(dlite)

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-2 main.c chemistry.h)
target_link_libraries(dlite-example-2 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-2 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
