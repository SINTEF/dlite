# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  cmake_policy(SET CMP0144 OLD)
endif()

project(dlite-example-4
  VERSION 0.1
  LANGUAGES Fortran C)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${DLITE_ROOT}/share/Modules
  ${DLITE_ROOT}/share/dlite/cmake
  )
find_package(dlite REQUIRED)

include(dliteCodeGen)

dlite_codegen(
  chemistry.f90
  fortran-module
  json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json?mode=r
  ENV_OPTIONS --build
  --build-root
  )

  
add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")

link_directories(${DLITE_LIBRARY_DIR})

if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -ffree-line-length-none")
endif()

if(WIN32)
  set(CMAKE_Fortran_STANDARD_LIBRARIES "user32.lib advapi32.lib crypt32.lib")
endif()

add_executable(dlite-example-4 chemistry.f90 main.f90)
target_link_libraries(dlite-example-4 ${DLITE_LIBRARIES} dlite-fortran)
target_include_directories(dlite-example-4 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


add_test(
  NAME dlite-example-4
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-4
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

if(WIN32)
  list(APPEND dlite_PATH ${CMAKE_CURRENT_BINARY_DIR})
else()
  list(APPEND dlite_LD_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()

set_property(TEST dlite-example-4 PROPERTY
  ENVIRONMENT "DLITE_ROOT=${DLITE_ROOT}")
if(UNIX)
set_property(TEST dlite-example-4 APPEND PROPERTY
  ENVIRONMENT "LD_LIBRARY_PATH=${DLITE_ROOT}/lib")
elseif(WIN32)
set_property(TEST dlite-example-4 APPEND PROPERTY
  ENVIRONMENT_MODIFICATION "path_list_prepend:$<JOIN:${DLITE_ROOT}/lib,\\\;>")
endif()
