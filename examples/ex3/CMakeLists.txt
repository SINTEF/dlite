# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)

project(dlite-example-3
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${DLITE_ROOT}/cmake
  ${DLITE_ROOT}/cmake/Modules
  ${DLITE_ROOT}/share/cmake
  ${DLITE_ROOT}/share/cmake/Modules
  )
find_package(dlite)

include(dliteCodeGen)

dlite_codegen(
  philibtable.h
  c-header
  json:${CMAKE_CURRENT_SOURCE_DIR}/PhilibTable.json?mode=r
  --storage-plugins=${DLITE_STORAGE_PLUGINS}
  )

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-3 main.c philibtable.h)
target_link_libraries(dlite-example-3 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-3 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

enable_testing()

add_test(
  NAME dlite-example-3
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-3
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

set_property(TEST dlite-example-3 PROPERTY
  ENVIRONMENT "DLITE_ROOT=${DLITE_ROOT}")
if(UNIX)
set_property(TEST dlite-example-3 APPEND PROPERTY
  ENVIRONMENT "LD_LIBRARY_PATH=${DLITE_ROOT}/lib")
elseif(WIN32)
set_property(TEST dlite-example-3 APPEND PROPERTY
  ENVIRONMENT "PATH=${DLITE_ROOT/lib};${PATH}")
endif()
