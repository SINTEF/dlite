# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.4)
include(CPack)


project(dlite
  VERSION 0.2
  LANGUAGES C)


# Options
option(BUILD_HDF5      "If not installed, download and compile HDF5"  ON)

# Append our cmake-modules to CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${dlite_SOURCE_DIR}/cmake)

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
  add_custom_target(memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure -T memcheck
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
# Since External download and configure steps of ExternalProject
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


#
# hdf5
# ====
find_package(HDF5 COMPONENTS C)
if((NOT HDF5_FOUND) AND BUILD_HDF5)
  message(STATUS "Download and build HDF5 as external package")

  set(hdf5_VERSION        "1.10.1")
  set(hdf5_NAME           "hdf5-${hdf5_VERSION}")
  set(hdf5_PREFIX         "${CMAKE_CURRENT_BINARY_DIR}/${hdf5_NAME}")
  set(hdf5_SOURCE_DIR     "${hdf5_PREFIX}/src/${hdf5_NAME}")
  set(hdf5_INSTALL_PREFIX "${hdf5_PREFIX}/local")

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
    ${hdf5_NAME}
    PREFIX            ${hdf5_PREFIX}
    URL               https://support.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.10.1.tar.gz
    URL_MD5           43a2f9466702fb1db31df98ae6677f15
    SOURCE_DIR        ${hdf5_SOURCE_DIR}
    UPDATE_COMMAND    ""
    CMAKE_ARGS        ${cmake_args}
    )

  # Define the following variables:
  #
  #   hdf5_BINARY_DIR   - root of hdf5 binary dir
  #   hdf5_INCLUDE_DIRS - location of the hdf5 include dirs
  #   hdf5_LIBRARY_DIRS - location of the hdf5 library dirs
  #   hdf5_LIBRARIES    - required libraries (for static linking)
  #
  set(hdf5_BINARY_DIR ${hdf5_INSTALL_PREFIX}/bin)
  set(hdf5_INCLUDE_DIRS ${hdf5_INSTALL_PREFIX}/include)
  set(hdf5_LIBRARY_DIRS ${hdf5_INSTALL_PREFIX}/lib)
  set(hdf5_LIBRARIES
    ${hdf5_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}hdf5${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
  if(WIN32)
    list(APPEND hdf5_LIBRARIES Kernel32)
  else()
    list(APPEND hdf5_LIBRARIES m dl)
  endif()

else()
  set(hdf5_LIBRARIES hdf5)
endif()



#
# uuid
# ====
set(uuid_SOURCE_DIR  ${dlite_SOURCE_DIR}/uuid)
set(cmake_args -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS})
ExternalProject_Add(
  uuidProj
  PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/uuid
  SOURCE_DIR      ${uuid_SOURCE_DIR}
  CMAKE_ARGS      ${cmake_args}
  UPDATE_COMMAND  ""
  TEST_COMMAND    ctest
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(uuidProj BINARY_DIR)
set(uuid_BINARY_DIR ${BINARY_DIR})

#################################################################


# Subdirectories
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(doc)
