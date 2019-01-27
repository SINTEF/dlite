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
  -ldlite -ldlite-utils -luuid
  )
target_include_directories(dlite-example-4 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
