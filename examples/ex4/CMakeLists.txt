# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)

project(dlite-example-4
  VERSION 0.1
  LANGUAGES Fortran C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(Finddlite)
find_package(dlite)


link_directories(${DLITE_LIBRARY_DIR})

if(WIN32)
  set(CMAKE_Fortran_STANDARD_LIBRARIES "user32.lib advapi32.lib crypt32.lib")
endif()

add_executable(dlite-example-4 main.f90 dlite.f90)
target_link_libraries(dlite-example-4
  ${DLITE_LIBRARIES}
  -ldlite -ldlite-utils
  )
target_include_directories(dlite-example-4 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


enable_testing()

set(test dlite-example-4)
add_test(
  NAME ${test}
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-4
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )


if(WIN32)
  list(APPEND dlite_PATH ${CMAKE_CURRENT_BINARY_DIR})
else()
  list(APPEND dlite_LD_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()

set_property(TEST ${test} PROPERTY
  ENVIRONMENT "PATH=${dlite_PATH}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
  "DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}")
set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
  "DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
set_property(TEST ${test} APPEND PROPERTY
  ENVIRONMENT "DLITE_STORAGES=${dlite_STORAGES}")
