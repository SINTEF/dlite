# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-1
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
    DLITE_STORAGE_PLUGINS="${DLITE_STORAGES_DIR}"
    ${DLITE_ROOT}/bin/dlite-codegen
    --template "${DLITE_TEMPLATES_DIR}/c-header.txt"
    --output chemistry.h
    --storage-plugins "${DLITE_STORAGES_DIR}"
    json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json?mode=r
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  COMMENT "Generating C header for Chemistry-0.1.json"
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
