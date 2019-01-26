# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)
#cmake_policy(VERSION 3.11)
#cmake_policy(SET CMP0074 NEW)
include(CPack)

project(dlite
  VERSION 0.2
  #DESCRIPTION "Lightweight library for working with scientific data"
  LANGUAGES C
  )


# Options
option(BUILD_HDF5   "If not installed, download and compile HDF5"    OFF)
option(BUILD_JSON   "If not installed, download and compile jansson" OFF)

option(WITH_HDF5    "Whether to build with HDF5 support"             ON)
option(WITH_JSON    "Whether to build with JSON support"             ON)


# Append our cmake-modules to CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${dlite_SOURCE_DIR}/cmake)

# Set defaults
include(SetDefaults)

# Enable compiler warnings
include(SetCompilerFlags)
if(CMAKE_COMPILER_IS_GNUCC)
  enable_c_compiler_flag_if_supported("-Wall")
  enable_c_compiler_flag_if_supported("-Wextra")
  enable_c_compiler_flag_if_supported("-Wpedantic")
endif()

# Uncomment the lines below to compile with AddressSanitizer
#set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")


# Testing
include(CTest)

find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)

if(CTEST_MEMORYCHECK_COMMAND)
  #set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE
  #  ${dlite_SOURCE_DIR}/cmake/valgrind-suppressions.txt)
  set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --trace-chrildren=yes --leak-check=full --track-origins=yes --suppressions=${dlite_SOURCE_DIR}/cmake/valgrind-suppressions.txt")
  add_custom_target(memcheck
    COMMAND ${CMAKE_CTEST_COMMAND}
              --force-new-ctest-process --output-on-failure -T memcheck
    COMMENT "Runs memory check with valgrind"
    )
endif()



####################################################################
# Use ExternalProject_add instead of add_subdirectory for packages.
# For explanation of the approach, see
# https://coderwall.com/p/y3zzbq/use-cmake-enabled-libraries-in-your-cmake-project
#
# In short, main reasons for using ExternalProject:
#   - Avoids problems with multible definitions of standard targets like
#     doc, memcheck, etc...
#   - External dependencies are transparently downloaded, configured
#     and compiled as a part of the build process.
#
# Since external download and configure steps of ExternalProject
# happens at compile time, find_package(), which operate at configure
# time, cannot be used with ExternalProject.  For this reason some
# people configures everything as ExternalProject (even the main
# project) and creates a super project that calls
# ExternalProject_Add() in the correct order such all dependencies are
# build when the ExternalProject of the main project is configured,
# allowing them to use configure project.
#
# Here we take another approach. Instead of relying on find_package()
# we manually sets the path of the resources (headers, libraries)
# provided by the dependency after ExternalProject_Add() is called.
# This way, the main project can follow the standard cmake structure.
#
####################################################################
include(ExternalProject)

# A directory that external projects install into
set(external_INSTALL_DIR ${dlite_BINARY_DIR}/external)


if(BUILD_SHARED_LIBS)
  set(LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
  set(LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

# Give the debug version a different postfix for windows,
# so both the debug and release version can be built in the
# same build-tree on Windows (MSVC).
if (WIN32 AND NOT CMAKE_DEBUG_POSTFIX)
   set(CMAKE_DEBUG_POSTFIX "_d")
endif()

# This should hopefully provide correct prefix and postfix for
# generated libraries
set(LIB_PREFIX ${LIBRARY_PREFIX})
#set(LIB_POSTFIX ${CMAKE_${CMAKE_BUILD_TYPE}_POSTFIX}${LIBRARY_SUFFIX})
set(LIB_POSTFIX ${LIBRARY_SUFFIX})


#
# hdf5
# ====
if(WITH_HDF5)
  find_package(HDF5 COMPONENTS C)
  if(NOT HDF5_FOUND)
    if(NOT BUILD_HDF5)
      message(FATAL_ERROR "HDF5 not found.  Try to set BUILD_HDF5 to ON")
    endif()
    message(STATUS "Download and build HDF5 as external package")

    set(hdf5_MAJOR_VERSION  "1.10")
    set(hdf5_MINOR_VERSION  "4")
    set(hdf5_MD5            cdf02e61f0d9920a7e7183aa0fb35429)
    set(hdf5_VERSION        "${hdf5_MAJOR_VERSION}.${hdf5_MINOR_VERSION}")
    set(hdf5_NAME           "hdf5-${hdf5_VERSION}")
    set(hdf5_SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/external/${hdf5_NAME}")
    set(hdf5_INSTALL_PREFIX "${external_INSTALL_DIR}")

    set(cmake_args
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DHDF5_BUILD_CXX=OFF
      -DHDF5_BUILD_FORTRAN=OFF
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_INSTALL_PREFIX=${hdf5_INSTALL_PREFIX}
      -DHAVE_IOEO_EXITCODE="FAILED_TO_RUN"
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    )
    if(BUILD_SHARED_LIBS)
      list(APPEND cmake_args "-DH5_BUILT_AS_DYNAMIC_LIB")
    endif()
    if(CMAKE_TOOLCHAIN_FILE)
      get_filename_component(toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
      list(APPEND cmake_args "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")
    endif()

    ExternalProject_Add(
      hdf5Proj
      PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/external/${hdf5_NAME}
      URL               https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-${hdf5_MAJOR_VERSION}/${hdf5_NAME}/src/${hdf5_NAME}.tar.gz
      URL_MD5           ${hdf5_MD5}
      SOURCE_DIR        ${hdf5_SOURCE_DIR}
      UPDATE_COMMAND    ""
      CMAKE_ARGS        ${cmake_args}
      TEST_COMMAND      ctest
      INSTALL_COMMAND   cmake --build . --target install
    )
    set(HDF5_DEPENDENCIES hdf5Proj)

    # Define the following variables:
    #
    #   HDF5_INCLUDE_DIRS - location of the HDF5 include dirs
    #   HDF5_LIBRARY_DIRS - location of the HDF5 library dirs
    #   HDF5_LIBRARIES    - required libraries (for static linking)
    #
    set(HDF5_INCLUDE_DIRS ${hdf5_INSTALL_PREFIX}/include)
    set(HDF5_LIBRARY_DIRS ${hdf5_INSTALL_PREFIX}/lib)
    set(HDF5_LIBRARIES
      ${hdf5_INSTALL_PREFIX}/lib/${LIB_PREFIX}hdf5${LIB_POSTFIX})

    if(WIN32)
      list(APPEND HDF5_LIBRARIES Kernel32)
    else()
      list(APPEND HDF5_LIBRARIES m dl)
    endif()

  endif()
endif()


#
# jansson
# =======
if(WITH_JSON)
  include(Findjansson)
  find_package(jansson)

  if(NOT JANSSON_FOUND)
    if(NOT BUILD_JSON)
      message(FATAL_ERROR "Jansson not found.  Try to set BUILD_JSON to ON")
    endif()
    message(STATUS "Download and build jansson as external package")
    set(jansson_SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/external/jansson")
    set(jansson_INSTALL_PREFIX "${external_INSTALL_DIR}")
    set(cmake_args
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_INSTALL_PREFIX=${jansson_INSTALL_PREFIX}
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
      )
    if(CMAKE_TOOLCHAIN_FILE)
      get_filename_component(toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
      list(APPEND cmake_args "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")
    endif()
    ExternalProject_Add(
      janssonProj
      PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/external/jansson
      SOURCE_DIR      ${jansson_SOURCE_DIR}
      GIT_REPOSITORY  "https://github.com/akheron/jansson.git"
      GIT_TAG         v2.9
      CMAKE_ARGS      ${cmake_args}
      UPDATE_COMMAND  ""
      TEST_COMMAND    ctest
      INSTALL_COMMAND cmake --build . --target install
      )
    set(JANSSON_DEPENDENCIES janssonProj)
    set(JANSSON_INCLUDE_DIRS ${jansson_INSTALL_PREFIX}/include)
    set(JANSSON_LIBRARIES
      ${jansson_INSTALL_PREFIX}/lib/${LIB_PREFIX}jansson${LIB_POSTFIX})
  endif()
endif()


#
# uuid
# ====
set(uuid_SOURCE_DIR ${dlite_SOURCE_DIR}/uuid)
set(uuid_BINARY_DIR ${dlite_BINARY_DIR}/uuid)
set(UUID_LIBRARIES
  ${external_INSTALL_DIR}/${LIB_PREFIX}uuid${LIB_POSTFIX})
set(UUID_STATIC_LIBRARIES
  ${external_INSTALL_DIR}/${LIB_PREFIX}uuid-static${CMAKE_STATIC_LIBRARY_SUFFIX})
set(UUID_INCLUDE_DIRS ${external_INSTALL_DIR}/include)
set(cmake_args
  -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_INSTALL_PREFIX=${external_INSTALL_DIR}
  )
ExternalProject_Add(
  uuidProj
  PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/uuid
  SOURCE_DIR        ${uuid_SOURCE_DIR}
  BINARY_DIR        ${uuid_BINARY_DIR}
  CMAKE_ARGS        ${cmake_args}
  UPDATE_COMMAND    ""
  BUILD_BYPRODUCTS  ${UUID_LIBRARIES} ${uuid_BINARY_DIR}/integers.h
  TEST_COMMAND      ctest
  INSTALL_COMMAND   cmake --build . --target install
)
set_source_files_properties(
    ${UUID_LIBRARIES}
    ${UUID_STATIC_LIBRARIES}
    ${uuid_BINARY_DIR}/integers.h
  PROPERTIES
    GENERATED uuidProj
  )


#################################################################


# Subdirectories
if(WITH_HDF5)
  add_subdirectory(plugins/hdf5)
endif()

if(WITH_JSON)
  add_subdirectory(plugins/json)
endif()

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(doc)


#################################################################


# Install
install(FILES
  README.md
  #LICENSE.txt
  DESTINATION share/dlite
  )

install(DIRECTORY
    ${dlite_BINARY_DIR}/install
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  )
