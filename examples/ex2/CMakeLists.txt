# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  cmake_policy(SET CMP0144 OLD)
endif()

project(dlite-example-2
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${DLITE_ROOT}/share/Modules
  ${DLITE_ROOT}/share/dlite/cmake
  )
find_package(dlite REQUIRED)

include(dliteCodeGen)

dlite_codegen(
  ${CMAKE_CURRENT_BINARY_DIR}/chemistry.h
  c-header
  json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json
  --storage-plugins=${DLITE_STORAGE_PLUGINS}
  )

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

link_directories(${DLITE_LIBRARY_DIR})

add_executable(dlite-example-2 main.c chemistry.h)
target_link_libraries(dlite-example-2 ${DLITE_LIBRARIES})
target_include_directories(dlite-example-2 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


add_test(
  NAME dlite-example-2
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-2
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

set_property(TEST dlite-example-2 PROPERTY
  ENVIRONMENT "DLITE_ROOT=${DLITE_ROOT}")
if(UNIX)
set_property(TEST dlite-example-2 APPEND PROPERTY
  ENVIRONMENT "LD_LIBRARY_PATH=${DLITE_ROOT}/lib")
elseif(WIN32)
set_property(TEST dlite-example-2 APPEND PROPERTY
  ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:${DLITE_ROOT}/lib,\\\;>")
endif()
