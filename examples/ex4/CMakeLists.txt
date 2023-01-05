# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)

find_package(dlite REQUIRED)

project(dlite-example-4
  VERSION 0.1
  LANGUAGES Fortran C)

add_compile_options("-DDLITE_ROOT=${DLITE_ROOT}")
#TODO: make this platform independent, need to add support for long strings
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -ffree-line-length-none")

list(APPEND CMAKE_MODULE_PATH
  ${DLITE_ROOT}/cmake
  ${DLITE_ROOT}/cmake/Modules
  ${DLITE_ROOT}/share/cmake
  ${DLITE_ROOT}/share/cmake/Modules
  ${DLITE_ROOT}/share/dlite/cmake
  )

link_directories(${DLITE_LIBRARY_DIR})

if(WIN32)
  set(CMAKE_Fortran_STANDARD_LIBRARIES "user32.lib advapi32.lib crypt32.lib")
endif()

include(dliteCodeGen)
dlite_codegen(
   chemistry.f90
   fortran-module
   json://${CMAKE_CURRENT_SOURCE_DIR}/Chemistry-0.1.json?mode=r
    ENV_OPTIONS --build
    --build-root
)

add_executable(dlite-example-4 chemistry.f90 main.f90)
target_link_libraries(dlite-example-4
   dlite
   dlite-utils
   dlite-fortran
  )
target_include_directories(dlite-example-4 PUBLIC
  ${DLITE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${DLITE_ROOT}/share/dlite/fortran
  )

enable_testing()

set(test dlite-example-4)
add_test(
  NAME ${test}
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/dlite-example-4
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )


# if(WIN32)
#   list(APPEND dlite_PATH ${CMAKE_CURRENT_BINARY_DIR})
# else()
#   list(APPEND dlite_LD_LIBRARY_PATH ${CMAKE_CURRENT_BINARY_DIR})
# endif()

# set_property(TEST ${test} PROPERTY
#   ENVIRONMENT "DLITE_ROOT=${DLITE_ROOT}")
# if(UNIX)
# set_property(TEST ${test} APPEND PROPERTY
#   ENVIRONMENT "LD_LIBRARY_PATH=${DLITE_ROOT}/lib")
# elseif(WIN32)
# set_property(TEST ${test} APPEND PROPERTY
#   ENVIRONMENT "PATH=${DLITE_ROOT/lib};${PATH}")
# endif()
