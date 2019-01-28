# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-3
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(Finddlite)
find_package(dlite)

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
OUTPUT philibtable.h
COMMAND ${DLITE_ROOT}/bin/dlite-codegen
  --template ${DLITE_TEMPLATE_DIR}/c-header.txt
  --driver-uri ${CMAKE_CURRENT_SOURCE_DIR}/PhilibTable.json
  --output philibtable.h
MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/PhilibTable.json
COMMENT "Generating C header for PhilibTable.json"
)

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-3 main.c philibtable.h)
target_link_libraries(dlite-example-3 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-3 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
