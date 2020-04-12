# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.11)
#cmake_policy(VERSION 3.11)
#cmake_policy(SET CMP0074 NEW)
include(CPack)

project(dlite
  VERSION 0.2.1
  #DESCRIPTION "Lightweight library for working with scientific data"
  LANGUAGES C
  )

# Options
option(WITH_PYTHON      "Whether to build Python 3 bindings"             ON)
option(WITH_HDF5        "Whether to build with HDF5 support"             ON)
option(WITH_JSON        "Whether to build with JSON support"             ON)
option(WITH_DOC         "Whether to build documentation using doxygen"   ON)
option(WITH_EXAMPLES    "Whether to build/run examples during testing"   ON)

option(FORCE_EXAMPLES   "Whether to force building/running examples"     OFF)

option(BUILD_HDF5       "If not installed, download and compile HDF5"    OFF)
option(BUILD_JSON       "If not installed, download and compile jansson" OFF)

# Append our cmake-modules to CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${dlite_SOURCE_DIR}/cmake)

# Set default cmake configurations
include(SetDefaults)


# Compiler options
# ----------------
# Enable compiler warnings
include(SetCompilerFlags)
if(CMAKE_COMPILER_IS_GNUCC)
  enable_c_compiler_flag_if_supported("-Wall")
  enable_c_compiler_flag_if_supported("-Wextra")
  enable_c_compiler_flag_if_supported("-Wpedantic")
  enable_c_compiler_flag_if_supported("-Werror")
endif()

# Enable C99
set(CMAKE_C_STANDARD 99)


# Uncomment the lines below to compile with AddressSanitizer
#set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")


# Install paths
# -------------
# DLite install paths (CMAKE_INSTALL_PREFIX) is prepended to these
set(DLITE_TEMPLATE_DIRS              "share/dlite/templates")
set(DLITE_STORAGE_PLUGIN_DIRS        "share/dlite/storage-plugins")
set(DLITE_MAPPING_PLUGIN_DIRS        "share/dlite/mapping-plugins")
set(DLITE_PYTHON_MAPPING_PLUGIN_DIRS "share/dlite/python-mapping-plugins")
set(DLITE_PYTHON_STORAGE_PLUGIN_DIRS "share/dlite/python-storage-plugins")
set(DLITE_STORAGES                   "share/dlite/storages")

# Install path for CMake files
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR cmake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/dlite)
endif()
set(DLITE_INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")


# Testing
# -------
find_program(MEMORYCHECK_COMMAND NAMES valgrind)

if(MEMORYCHECK_COMMAND)
  set(MEMORYCHECK_SUPPRESSIONS_FILE
    ${dlite_SOURCE_DIR}/cmake/valgrind-suppressions.txt
    CACHE FILEPATH "File that contains suppressions for the memory checker"
    )
  set(MEMORYCHECK_COMMAND_OPTIONS
    "-q --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=50")
  #set(MEMORYCHECK_COMMAND_OPTIONS
  #  "-q --tool=memcheck --leak-check=yes --show-leak-kinds=definite
  #   --gen-suppressions=all")

  add_custom_target(memcheck
    COMMAND ${CMAKE_CTEST_COMMAND}
              --force-new-ctest-process --output-on-failure -T memcheck
    COMMENT "Runs memory check with valgrind"
    )
endif()

include(CTest)

find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_test(
    NAME static-code-analysis
    COMMAND ${CPPCHECK} ${CMAKE_CURRENT_SOURCE_DIR}
      --language=c -q --force --error-exitcode=2 --inline-suppr -i build
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
set(external_INSTALL_DIR ${dlite_BINARY_DIR}/local)

# Short names
set(LIB_PRE ${CMAKE_STATIC_LIBRARY_PREFIX})
set(LIB_EXT ${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SH_PRE  ${CMAKE_SHARED_LIBRARY_PREFIX})
set(SH_EXT  ${CMAKE_SHARED_LIBRARY_SUFFIX})

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
      -DBUILD_SHARED_LIBS=ON
      -DH5_BUILT_AS_DYNAMIC_LIB
    )
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
      TEST_COMMAND      ctest -C Release
      INSTALL_COMMAND   cmake --build . --target install
    )
    set(HDF5_DEPENDENCIES hdf5Proj)

    # Define the following variables:
    #
    #   HDF5_INCLUDE_DIRS - location of the HDF5 include dirs
    #   HDF5_LIBRARY_DIRS - location of the HDF5 library dirs
    #   HDF5_LIBRARIES    - required libraries
    #
    set(HDF5_INCLUDE_DIRS ${hdf5_INSTALL_PREFIX}/include)
    set(HDF5_LIBRARY_DIRS ${hdf5_INSTALL_PREFIX}/lib)
    set(HDF5_LIBRARIES
      ${hdf5_INSTALL_PREFIX}/lib/${SH_PRE}hdf5${SH_EXT})

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
      -DBUILD_SHARED_LIBS=ON
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
      ${jansson_INSTALL_PREFIX}/lib/${SH_PRE}jansson${SH_EXT})
  endif()
endif()


#
# Python
# ======
if(WITH_PYTHON)
  if(CMAKE_VERSION VERSION_LESS_EQUAL 3.12)
    set(Python_ADDITIONAL_VERSIONS 3.8 3.7 3.6 3.5)
    find_package(PythonInterp 3.5 REQUIRED)
    find_package(PythonLibs 3.5 REQUIRED)
  else()
    if(CMAKE_VERSION VERSION_LESS 3.14)
      find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
      set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
      find_package(Numpy REQUIRED)
    else()
      find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
    endif()

    # Set backward compatible Python variables
    set(PYTHONINTERP_FOUND    ${Python3_Interpreter_FOUND})
    set(PYTHON_EXECUTABLE     ${Python3_EXECUTABLE})

    set(PYTHON_VERSION_STRING ${Python3_VERSION})
    set(PYTHON_VERSION_MAJOR  ${Python3_VERSION_MAJOR})
    set(PYTHON_VERSION_MINOR  ${Python3_VERSION_MINOR})
    set(PYTHON_VERSION_PATCH  ${Python3_VERSION_PATCH})

    set(PYTHONLIBS_FOUND      ${Python3_Development_FOUND})
    set(PYTHON_LIBRARIES      ${Python3_LIBRARIES})
    set(PYTHON_INCLUDE_DIRS   ${Python3_INCLUDE_DIRS})
    set(PYTHON_LIBRARY_DIRS   ${Python3_LIBRARY_DIRS})
    set(PYTHONLIBS_VERSION_STRING ${Python3_VERSION})
  endif()
endif()


# -------------------------------------------
# Environment for running tests in build tree
# -------------------------------------------

# PATH - path to runtime directories (.exe and .dll)
set(dlite_PATH ${dlite-tools_BINARY_DIR})
if(WIN32)
  list(APPEND dlite_PATH
    ${dlite_BINARY_DIR}/src
    ${dlite_BINARY_DIR}/src/utils
    ${JANSSON_RUNTIME_DIR}
    ${HDF5_RUNTIME_DIR}
    )
  string(REPLACE "/" "\\" dlite_PATH "${dlite_PATH}")
endif()
list(APPEND dlite_PATH
  $ENV{PATH}
  )
list(REMOVE_DUPLICATES dlite_PATH)

# LD_LIBRARY_PATH - path to shared library directories (Linux only)
set(dlite_LD_LIBRARY_PATH
  ${dlite_BINARY_DIR}/src
  ${dlite_BINARY_DIR}/src/utils
  ${JANSSON_LIBRARY_DIR}
  ${HDF5_LIBRARY_DIR}
  $ENV{LD_LIBRARY_PATH}
  )
list(REMOVE_DUPLICATES dlite_LD_LIBRARY_PATH)

# PYTHONPATH - Python search path for modules and packages
set(dlite_PYTHONPATH
  ${dlite_BINARY_DIR}/bindings/python
  $ENV{PYTHONPATH}
  )
if(dlite_PYTHONPATH)
  list(REMOVE_DUPLICATES dlite_PYTHONPATH)
endif()

# DLITE_STORAGE_PLUGIN_DIRS - search path for DLite storage plugins
set(dlite_STORAGE_PLUGINS
  ${dlite_BINARY_DIR}/storages/json
  ${dlite_BINARY_DIR}/storages/hdf5
  ${dlite_BINARY_DIR}/storages/python
  $ENV{DLITE_STORAGE_PLUGIN_DIRS}
  )
list(REMOVE_DUPLICATES dlite_STORAGE_PLUGINS)

# DLITE_MAPPING_PLUGIN_DIRS - search path for DLite mapping plugins
set(dlite_MAPPING_PLUGINS
  ${dlite_BINARY_DIR}/src/pyembed
  $ENV{DLITE_MAPPING_PLUGIN_DIRS}
  )
list(REMOVE_DUPLICATES dlite_MAPPING_PLUGINS)

# DLITE_PYTHON_STORAGE_PLUGIN_DIRS - search path for Python storage plugins
set(dlite_PYTHON_STORAGE_PLUGINS
  ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins
  $ENV{DLITE_PYTHON_STORAGE_PLUGIN_DIRS}
  )
list(REMOVE_DUPLICATES dlite_PYTHON_STORAGE_PLUGINS)

# DLITE_PYTHON_MAPPING_PLUGIN_DIRS - search path for Python mapping plugins
set(dlite_PYTHON_MAPPING_PLUGINS
  ${dlite_SOURCE_DIR}/mapping-plugins/src/pyembed
  $ENV{DLITE_PYTHON_MAPPING_PLUGIN_DIRS}
  )
list(REMOVE_DUPLICATES dlite_PYTHON_MAPPING_PLUGINS)

# DLITE_TEMPLATE_DIRS - search path for DLite templates
set(dlite_TEMPLATES
  ${dlite_SOURCE_DIR}/tools/templates
  $ENV{DLITE_TEMPLATE_DIRS}
  )
list(REMOVE_DUPLICATES dlite_TEMPLATES)

# DLITE_STORAGES - DLite storages (inc. metadata)
set(dlite_STORAGES
  ${dlite_SOURCE_DIR}/examples/storages/*.json
  $ENV{DLITE_STORAGES}
  )
list(REMOVE_DUPLICATES dlite_STORAGES)

if(UNIX)
  string(REPLACE ";" ":" dlite_PATH             "${dlite_PATH}")
  string(REPLACE ";" ":" dlite_LD_LIBRARY_PATH  "${dlite_LD_LIBRARY_PATH}")
  string(REPLACE ";" ":" dlite_PYTHONPATH       "${dlite_PYTHONPATH}")
  string(REPLACE ";" ":" dlite_STORAGE_PLUGINS  "${dlite_STORAGE_PLUGINS}")
  string(REPLACE ";" ":" dlite_MAPPING_PLUGINS  "${dlite_MAPPING_PLUGINS}")
  string(REPLACE ";" ":" dlite_PYTHON_STORAGE_PLUGINS  "${dlite_PYTHON_STORAGE_PLUGINS}")
  string(REPLACE ";" ":" dlite_PYTHON_MAPPING_PLUGINS  "${dlite_PYTHON_MAPPING_PLUGINS}")
  string(REPLACE ";" ":" dlite_TEMPLATES        "${dlite_TEMPLATES}")
endif()

# Note, DLITE_STORAGES uses "|" as separator on all systems
string(REPLACE ";" "|" dlite_STORAGES         "${dlite_STORAGES}")

#################################################################

# Report current settings
message("")
message("========== Test environment =========")
message("export PATH=${dlite_PATH}")
message("export LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
message("export PYTHONPATH=${dlite_PYTHONPATH}")
message("export DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}")
message("export DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}")
message("export DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}")
message("export DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}")
message("export DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}")
message("export DLITE_STORAGES='${dlite_STORAGES}'")
message("")
message("========== Python ==================")
message("PYTHON_INCLUDE_DIRS:  ${PYTHON_INCLUDE_DIRS}")
message("PYTHON_LIBRARIES:     ${PYTHON_LIBRARIES}")
message("PYTHON_LIBRARY_DIRS:  ${PYTHON_LIBRARY_DIRS}")
message("")
message("========== Jansson ==================")
message("JANSSON_INCLUDE_DIRS: ${JANSSON_INCLUDE_DIRS}")
message("JANSSON_LIBRARIES:    ${JANSSON_LIBRARIES}")
message("JANSSON_LIBRARY_DIR:  ${JANSSON_LIBRARY_DIR}")
message("JANSSON_RUNTIME_DIR:  ${JANSSON_RUNTIME_DIR}")
message("")
message("========== HDF5 =====================")
message("HDF5_INCLUDE_DIRS:    ${HDF5_INCLUDE_DIRS}")
message("HDF5_LIBRARIES:       ${HDF5_LIBRARIES}")
message("HDF5_LIBRARY_DIR:     ${HDF5_LIBRARY_DIR}")
message("HDF5_RUNTIME_DIR:     ${HDF5_RUNTIME_DIR}")
message("=====================================")

#################################################################

# Subdirectories
add_subdirectory(src)
add_subdirectory(tools)

# Bindings
if(WITH_PYTHON)
  add_subdirectory(bindings/python)
endif()

# Storage plugins
if(WITH_HDF5)
  add_subdirectory(storages/hdf5)
endif()
if(WITH_JSON)
  add_subdirectory(storages/json)
endif()
if(WITH_PYTHON)
  add_subdirectory(storages/python)
endif()

# Examples
if(WITH_EXAMPLES)
  add_subdirectory(examples)
endif()

# Documentations
if(WITH_DOC)
  add_subdirectory(doc)
endif()

#################################################################

## Generate the config file for the installation tree.
include(CMakePackageConfigHelpers)

export(
  TARGETS dlite-utils dlite-utils-static dlite dlite-static
  #NAMESPACE dlite::
  #FILE "${CMAKE_CURRENT_BINARY_DIR}/dliteConfigExport.cmake"
  FILE "${CMAKE_CURRENT_BINARY_DIR}/dliteTargets.cmake"
  )

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dliteConfigVersion.cmake"
  VERSION ${dlite_VERSION}
  COMPATIBILITY SameMajorVersion
  )

set(DLITE_ROOT ${CMAKE_INSTALL_PREFIX})
set(DLITE_INCLUDE_DIRS include/dlite)
set(DLITE_LIBRARY_DIR  lib)
set(DLITE_RUNTIME_DIR  bin)
#set(python_LIBRARIES $<$<BOOL:${PYTHON_LIBRARIES}>:PYTHON_LIBRARIES>)
#set(jansson_LIBRARIES $<$<BOOL:${JANSSON_LIBRARIES}>:JANSSON_LIBRARIES>)
#set(hdf5_LIBRARIES $<$<BOOL:${HDF5_LIBRARIES}>:HDF5_LIBRARIES>)

configure_package_config_file(
  cmake/dliteConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/dliteConfig.cmake"
  INSTALL_DESTINATION "${DLITE_INSTALL_CMAKE_DIR}"
  PATH_VARS
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_TEMPLATE_DIRS
    DLITE_STORAGE_PLUGIN_DIRS
    DLITE_MAPPING_PLUGIN_DIRS
    DLITE_PYTHON_STORAGE_PLUGIN_DIRS
    DLITE_PYTHON_MAPPING_PLUGIN_DIRS
    DLITE_STORAGES
    PYTHON_LIBRARIES
    JANSSON_LIBRARIES
    HDF5_LIBRARIES
  )

#################################################################

# Install
install(
  FILES
    README.md
    LICENSE.txt
  DESTINATION share/dlite
  )

install(
  EXPORT dliteTargets
  DESTINATION "${DLITE_INSTALL_CMAKE_DIR}"
  #NAMESPACE dlite::
  )
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dliteConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dliteConfig.cmake"
  DESTINATION "${DLITE_INSTALL_CMAKE_DIR}"
  )
