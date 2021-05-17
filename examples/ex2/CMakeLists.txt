# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)

project(dlite-example-2
  VERSION 0.1
  LANGUAGES C)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
#include(Finddlite)
find_package(dlite REQUIRED)

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

add_custom_command(
  OUTPUT chemistry.h
  COMMAND
    ${CMAKE_COMMAND} -E env
    LD_LIBRARY_PATH="${DLITE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}"
    DLITE_STORAGE_PLUGIN_DIRS="${DLITE_STORAGE_PLUGIN_DIRS}"
    ${DLITE_ROOT}/bin/dlite-codegen
      --format=c-header
      --output=chemistry.h
      --storage-plugins="${DLITE_STORAGE_PLUGIN_DIRS}"
      json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json?mode=r
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  COMMENT "Generating C header for Chemistry-0.1.json"
  )

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-2 main.c chemistry.h)
target_link_libraries(dlite-example-2 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-2 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


enable_testing()

add_test(
  NAME dlite-example-2
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-2
  )

set_property(TEST dlite-example-2 PROPERTY
  ENVIRONMENT "DLITE_ROOT=${DLITE_ROOT}")
if(UNIX)
set_property(TEST dlite-example-2 APPEND PROPERTY
  ENVIRONMENT "LD_LIBRARY_PATH=${DLITE_ROOT}/lib")
elseif(WIN32)
set_property(TEST dlite-example-2 APPEND PROPERTY
  ENVIRONMENT "PATH=${DLITE_ROOT/lib};${PATH}")
endif()
