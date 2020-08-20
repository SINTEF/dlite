# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-1
  VERSION 0.1
  LANGUAGES C)

find_package(dlite REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DLITE_ROOT}/share/dlite/cmake)

include(dliteCodeGen)

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

dlite_codegen(
  chemistry.h
  c-header
  json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json?mode=r
  )


link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-1 main.c chemistry.h)
target_link_libraries(dlite-example-1 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-1 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


enable_testing()

add_test(
  NAME dlite-example-1
  COMMAND dlite-example-1
  )
