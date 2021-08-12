# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)

project(dlite
  VERSION 0.3.0
  DESCRIPTION "Lightweight data-centric framework for working with scientific data"
  HOMEPAGE_URL "https://github.com/SINTEF/dlite"
  LANGUAGES C
  )
set(dlite_VENDOR "SINTEF")
set(dlite_LICENSE "MIT")
set(dlite_CONTACT "Jesper Friis")
set(dlite_LOGO "doc/figs/logo_circle.svg")
set(dlite_LONG_DESCRIPTION "\
DLite is a lightweight cross-platform C library, for working with and
sharing scientific data in an interoperable way.

All data in DLite is represented by an Instance, which is build on a
simple data model.  An Instance is identified by a unique UUID and
have a set of named dimensions and properties.  It is described by its
Metadata.  In the Metadata, each dimension is given a name and
description (optional) and each property is given a name, type, shape
(optional), unit (optional) and description (optional).  The shape of
a property refers to the named dimensions.

When an Instance is instantiated, you must suply a value to the named
dimensions.  The shape of the properties will be set according to
that.  This ensures that the shape of the properties are internally
consistent.

A Metadata is also an Instance, and hence described by its
meta-metadata.  By default, DLite defines four levels of metadata;
instance, metadata, metadata schema and basic metadata schema. The
basic metadata schema describes itself, so no further meta levels are
needed.  The idea is if two different systems describes their data
model in terms of the basic metadata schema, they can easily be made
semantically interoperable.

An alternative and more flexible way to enable interoperability is to
use a common ontology.  DLite provides a specialised Instance called
Collection.  A collection is essentially a container holding a set of
Instances and relations between them.  But it can also relate an
Instance or even a dimension or property of an instance to a concept
in an ontology.  DLite allows to transparently map an Instance whos
Metadata corresponding to a concept in one ontology to an Instance
whos Metadata corresponding to a concept in another ontology.  Such
mappings can easily be registered (in C or Python) and reused,
providing a very powerful system for achieving interoperability.

DLite provides also a common and extendable API for loading/storing
Instances from/to different storages.  New storage plugins can be
written in C or Python.

DLite is released under the MIT license.
")

# Options
option(WITH_PYTHON               "Whether to build Python 3 bindings"             ON)
option(WITH_PYTHON_BUILD_WHEEL   "Whether to build the Python wheel"              OFF)
option(WITH_PYTHON_TEST_WHEEL    "Whether to run tests in venv."                  OFF)
option(WITH_FORTRAN              "Whether to build Fortran bindings"              OFF)
option(WITH_HDF5                 "Whether to build with HDF5 support"             ON)
option(WITH_JSON                 "Whether to build with JSON support"             ON)
option(WITH_REDLAND              "Whether to build with Redland (if available)"   ON)
option(WITH_DOC                  "Whether to build documentation using doxygen"   ON)
option(WITH_EXAMPLES             "Whether to build/run examples during testing"   ON)
option(FORCE_EXAMPLES            "Whether to force building/running examples"     OFF)
option(ALLOW_WARNINGS            "Whether to not fail on compilation warnings"    OFF)

option(BUILD_HDF5                "If not installed, download and compile HDF5"    OFF)
option(BUILD_JSON                "If not installed, download and compile jansson" OFF)


# Append our cmake-modules to CMAKE_MODULE_PATH
list(APPEND CMAKE_MODULE_PATH ${dlite_SOURCE_DIR}/cmake)

# Enable C99
set(CMAKE_C_STANDARD 99)

# Set default cmake configurations
include(SetDefaults)

# Get number of CPUs
include(ProcessorCount)
ProcessorCount(NUMBER_OF_CPUS)
if(NUMBER_OF_CPUS)
  set(NUMBER_OF_CPUS_OPTION -j${NUMBER_OF_CPUS})
else()
  set(NUMBER_OF_CPUS_OPTION "")
endif()

# Compiler options
# ----------------
include(SetCompilerFlags)

enable_c_compiler_flag_if_supported("-m64")

# Enable compiler warnings
if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions(-D_GNU_SOURCE)
  enable_c_compiler_flag_if_supported("-Wall")
  enable_c_compiler_flag_if_supported("-Wextra")
  enable_c_compiler_flag_if_supported("-Wpedantic")
  execute_process(COMMAND gcc -dumpfullversion OUTPUT_VARIABLE gcc_version)
  if(NOT ALLOW_WARNINGS AND "${gcc_version}" VERSION_GREATER_EQUAL 9.0)
    enable_c_compiler_flag_if_supported("-Werror")
  endif()
endif()

# Define MINGW if we are running a GNU compiler on a Windows system
if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(MINGW TRUE)
  enable_c_compiler_flag_if_supported("-Wno-pragmas")
  enable_c_compiler_flag_if_supported("-Wno-unknown-pragmas")
endif()

# Fix utf-8 handling for MSVC: Force utf-8 encoded source-code files
if(MSVC)
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

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
  set(DLITE_INSTALL_CMAKE_DIR "cmake")
else()
  set(DLITE_INSTALL_CMAKE_DIR "share/dlite/cmake")
endif()

# Variables to include in dliteConfig.cmake
set(DLITE_CONFIG_VARS
  dlite_VERSION
  DLITE_ROOT
  DLITE_INCLUDE_DIRS
  DLITE_LIBRARY_DIR
  DLITE_RUNTIME_DIR
  DLITE_TEMPLATE_DIRS
  DLITE_STORAGE_PLUGIN_DIRS
  DLITE_MAPPING_PLUGIN_DIRS
  )
  if(WITH_JSON)
    list(APPEND DLITE_CONFIG_VARS JANSSON_LIBRARIES)
  endif()
  if(WITH_HDF5)
    list(APPEND DLITE_CONFIG_VARS HDF5_LIBRARIES)
  endif()
  if(WITH_PYTHON)
    list(APPEND DLITE_CONFIG_VARS
      Python3_LIBRARIES
      DLITE_PYTHON_STORAGE_PLUGIN_DIRS
      DLITE_PYTHON_MAPPING_PLUGIN_DIRS
      )
  endif()

# ------------------------
# Installation environment
# ------------------------

# Define installation directory names according to the GNU standards
include(GNUInstallDirs)

# Installation paths
set(DLITE_ROOT ${CMAKE_INSTALL_PREFIX})
set(DLITE_INCLUDE_DIRS include/dlite)
set(DLITE_LIBRARY_DIR  lib)
set(DLITE_RUNTIME_DIR  bin)
set(DLITE_DATA_DIR     share/dlite)

include(MakePlatformPaths)
make_platform_paths(
  PATHS
    DLITE_ROOT
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_DATA_DIR
  )



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
  find_package(HDF5 1.8.20 COMPONENTS C)
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
else()
  set(HDF5_LIBRARIES "")
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
    if(NOT WIN32 OR MINGW)
    set(cmake_args
      -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX=${jansson_INSTALL_PREFIX}
      -DJANSSON_BUILD_DOCS=${WITH_DOC}
      -DBUILD_SHARED_LIBS=ON
      )
    else() # WIN32
    set(cmake_args
      -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX=${jansson_INSTALL_PREFIX}
      -DJANSSON_BUILD_DOCS=${WITH_DOC}
      -DBUILD_SHARED_LIBS=OFF
      -DJANSSON_BUILD_SHARED_LIBS=OFF
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    )
    endif()
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
      INSTALL_COMMAND cmake --build . --config Release --target install
      )
    set(JANSSON_DEPENDENCIES janssonProj)
    set(JANSSON_INCLUDE_DIRS ${jansson_INSTALL_PREFIX}/include)
    if(NOT WIN32 OR MINGW)
    set(JANSSON_LIBRARIES
      ${jansson_INSTALL_PREFIX}/lib/${SH_PRE}jansson${SH_EXT})
    else() # WIN32
    set(JANSSON_LIBRARIES
      ${jansson_INSTALL_PREFIX}/lib/${SH_PRE}jansson${LIB_EXT})
    endif()
  endif()
else()
  set(JANSSON_LIBRARIES "")
endif()


#
# Redland
# =======
if(WITH_REDLAND)
  find_package(Redland)
  if(REDLAND_FOUND)
    set(HAVE_REDLAND TRUE)

    find_package(Rasqal REQUIRED)
    if(RASQAL_FOUND)
      set(HAVE_RASQAL TRUE)
    endif()

    find_package(Raptor REQUIRED)
    if(RAPTOR_FOUND)
      set(HAVE_RAPTOR TRUE)
    endif()

  endif()
endif()


#
# Python
# ======
if(WITH_PYTHON)
  if(CROSS_TARGET AND MINGW)
    set(Python3_FIND_REGISTRY NEVER)
    set(Python3_FIND_STRATEGY LOCATION)
    set(Python3_ROOT_DIR /usr/${TOOLCHAIN_PREFIX})
  endif()

  set(CMAKE_CROSSCOMPILING_EMULATOR ${RUNNER})
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
  find_package(NumPy REQUIRED)
  unset(CMAKE_CROSSCOMPILING_EMULATOR)

  if(MINGW)
    # On mingw, python depends on libffi
    find_library(ffi ffi)
    mark_as_advanced(ffi)
    get_filename_component(dlite_PATH_EXTRA ${ffi} DIRECTORY)
  endif()

  if(MSVC)
    # Testing the python-bindings can fail on Windows for a number of reasons:
    # embedding: https://github.com/pybind/pybind11/issues/2369
    # numpy-related: https://github.com/numpy/numpy/issues/12667
    # Test in a virtual environment instead.
    set(WITH_PYTHON_VENVTEST_ONLY ON)
  endif()

  set(DLITE_PYTHONPATH
    ${DLITE_LIBRARY_DIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages)


else()
  set(Python3_LIBRARIES "")
endif()


#
# Fortran
# =======
if(WITH_FORTRAN)
  enable_language(Fortran)

  enable_fortran_compiler_flag_if_supported("-std=f2008")

  enable_fortran_compiler_flag_if_supported("-Wall")
  enable_fortran_compiler_flag_if_supported("-Wextra")
  enable_fortran_compiler_flag_if_supported("-Wpedantic")
  enable_fortran_compiler_flag_if_supported("-Werror")
endif()


# Testing
# -------
set(logfile "${CMAKE_CURRENT_BINARY_DIR}/valgrind_python.log")
set(suppfile "${CMAKE_CURRENT_BINARY_DIR}/valgrind_python.supp")
set(dlite_supp "${dlite_SOURCE_DIR}/cmake/valgrind-dlite.supp")
set(python_supp "${dlite_SOURCE_DIR}/cmake/valgrind-python.supp")

# Must be assigned before including CTest
set(MEMORYCHECK_COMMAND_OPTIONS "-q --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=200 --keep-debuginfo=yes --suppressions=${suppfile} --gen-suppressions=all")

include(CTest)

find_program(MEMORYCHECK_COMMAND NAMES valgrind)

if(MEMORYCHECK_COMMAND)
  if(WITH_PYTHON)
    set(suppressions ${suppfile})

    # Generate platform-specific suppressions file for Python
    add_custom_command(
      OUTPUT ${logfile}
      COMMAND ${CMAKE_COMMAND}
        -E env PYTHONMALLOC=malloc ${MEMORYCHECK_COMMAND}
        --leak-check=yes
        --show-leak-kinds=all
        --keep-debuginfo=yes
        --suppressions=${dlite_supp}
        --gen-suppressions=all
        --log-file=${logfile}
        ${Python3_EXECUTABLE} ${dlite_SOURCE_DIR}/cmake/valgrind_python.py
      MAIN_DEPENDENCY ${dlite_SOURCE_DIR}/cmake/valgrind_python.py
      COMMENT "Generating valgrind suppressions log file for Python"
      )
    add_custom_command(
      OUTPUT ${suppfile}
      COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/valgrind-log2supp.py
        --head=${dlite_supp}
        --tail=${python_supp}
        ${logfile}
        ${suppfile}
      MAIN_DEPENDENCY ${logfile}
      DEPENDS
        ${dlite_supp}
        ${python_supp}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/valgrind-log2supp.py
      COMMENT "Convert valgrind log file to suppressions for Python"
      )
  else()
    set(suppressions ${dlite_supp})
  endif()

  set(MEMORYCHECK_SUPPRESSIONS_FILE
    ${suppressions}
    CACHE FILEPATH "File that contains suppressions for the memory checker"
    )

  add_custom_target(memcheck
    COMMAND ${CMAKE_COMMAND}
        -E env PYTHONMALLOC=malloc ${CMAKE_CTEST_COMMAND}
            --exclude-regex static-code-analysis
            --force-new-ctest-process
            --output-on-failure
            --test-action memcheck
    DEPENDS ${suppressions}
    COMMENT "Runs memory check with valgrind"
    )
endif()


find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_test(
    NAME static-code-analysis
    COMMAND ${CPPCHECK} ${CMAKE_CURRENT_SOURCE_DIR}
      --language=c -q --force --error-exitcode=2 --inline-suppr -i build
      -I${dlite_SOURCE_DIR}/src ${NUMBER_OF_CPUS_OPTION}
    )
endif()



# -------------------------------------------
# Environment for running tests in build tree
# -------------------------------------------

# Macro that appends build directories `dirs` to list `lst`
#
# Usage: build_append(<var> dirs...)
#
# This macro is a workaround, since we cannot use $<TARGET_FILE_DIR:tgt>
# generator expressions because they makes the generated
# src/config-paths.h depend on the dlite libraries creating circular
# dependencies (since the libraries depends on src/config-paths.h).
macro(build_append lst)
  foreach(dir ${ARGN})
    list(APPEND ${lst} ${dir})
    if(WIN32)
      list(APPEND ${lst} ${dir}/Release)
      list(APPEND ${lst} ${dir}/Debug)
    endif()
  endforeach()
endmacro()

# PATH - path to runtime directories (.exe and .dll)
set(dlite_PATH "")
if(WIN32)
  build_append(dlite_PATH
    ${dlite_BINARY_DIR}/src
    ${dlite_BINARY_DIR}/src/utils
    ${dlite_BINARY_DIR}/tools
    )
  if(WITH_PYTHON)
    build_append(dlite_PATH ${dlite_BINARY_DIR}/src/pyembed)
    list(APPEND dlite_PATH ${Python3_NumPy_LIBRARY_DIRS})
  endif()
  if(WITH_FORTRAN)
    build_append(dlite_PATH ${dlite_BINARY_DIR}/bindings/fortran)
  endif()
  if(WITH_JSON)
    list(APPEND dlite_PATH ${JANSSON_RUNTIME_DIR})
  endif()
  if(WITH_HDF5)
    list(APPEND dlite_PATH ${HDF5_RUNTIME_DIR})
  endif()
  if(HAVE_REDLAND)
    list(APPEND dlite_PATH ${REDLAND_RUNTIME_DIR})
  endif()
endif()
list(REMOVE_DUPLICATES dlite_PATH)

# WINEPATH - path to runtime dirs (.exe and .dll) when cross-compiling for win64
if(CROSS_TARGET STREQUAL "win64" OR CROSS_TARGET STREQUAL "win32")
  set(dlite_WINEPATH "")
  build_append(dlite_WINEPATH
    ${dlite_BINARY_DIR}/src
    ${dlite_BINARY_DIR}/src/utils
    ${dlite_BINARY_DIR}/tools
    )
  if(WITH_PYTHON)
    build_append(dlite_WINEPATH ${dlite_BINARY_DIR}/src/pyembed)
    list(APPEND dlite_WINEPATH ${dlite_PATH_EXTRA})
    list(APPEND dlite_WINEPATH ${TOOLCHAIN_BINDIR})
    list(APPEND dlite_WINEPATH ${TOOLCHAIN_LIBDIR})
    list(APPEND dlite_WINEPATH ${Python3_NumPy_LIBRARY_DIRS})
  endif()
  if(WITH_FORTRAN)
    list(APPEND dlite_WINEPATH ${dlite_BINARY_DIR}/bindings/fortran)
  endif()
  if(WITH_JSON)
    list(APPEND dlite_WINEPATH ${JANSSON_RUNTIME_DIR})
  endif()
  if(WITH_HDF5)
    list(APPEND dlite_WINEPATH ${HDF5_RUNTIME_DIR})
  endif()
  if(HAVE_REDLAND)
    list(APPEND dlite_WINEPATH ${REDLAND_RUNTIME_DIR})
  endif()
  string(REPLACE ";" "\\;" dlite_WINEPATH_NATIVE "${dlite_WINEPATH}")
  list(REMOVE_DUPLICATES dlite_WINEPATH)
endif()

# LD_LIBRARY_PATH - path to shared library directories (Linux only)
set(dlite_LD_LIBRARY_PATH "")
build_append(dlite_LD_LIBRARY_PATH
  ${dlite_BINARY_DIR}/src
  ${dlite_BINARY_DIR}/src/utils
  )
if(WITH_PYTHON)
  build_append(dlite_LD_LIBRARY_PATH ${dlite_BINARY_DIR}/src/pyembed)
endif()
#list(APPEND dlite_LD_LIBRARY_PATH
#  ${JANSSON_RUNTIME_DIR}
#  ${HDF5_RUNTIME_DIR}
#  )
list(REMOVE_DUPLICATES dlite_LD_LIBRARY_PATH)

# PYTHONPATH - Python search path for modules and packages
set(dlite_PYTHONPATH
  ${dlite_BINARY_DIR}/bindings/python
  )
if(dlite_PYTHONPATH)
  list(REMOVE_DUPLICATES dlite_PYTHONPATH)
endif()

# DLITE_STORAGE_PLUGIN_DIRS - search path for DLite storage plugins
set(dlite_STORAGE_PLUGINS "")
if(WITH_JSON)
  build_append(dlite_STORAGE_PLUGINS ${dlite_BINARY_DIR}/storages/json)
endif()
if(WITH_HDF5)
  build_append(dlite_STORAGE_PLUGINS ${dlite_BINARY_DIR}/storages/hdf5)
endif()
if(HAVE_REDLAND)
  build_append(dlite_STORAGE_PLUGINS ${dlite_BINARY_DIR}/storages/rdf)
endif()
if(WITH_PYTHON)
  build_append(dlite_STORAGE_PLUGINS ${dlite_BINARY_DIR}/storages/python)
endif()
list(REMOVE_DUPLICATES dlite_STORAGE_PLUGINS)

# DLITE_MAPPING_PLUGIN_DIRS - search path for DLite mapping plugins
set(dlite_MAPPING_PLUGINS "")
list(REMOVE_DUPLICATES dlite_MAPPING_PLUGINS)

# DLITE_PYTHON_STORAGE_PLUGIN_DIRS - search path for Python storage plugins
set(dlite_PYTHON_STORAGE_PLUGINS
  ${dlite_SOURCE_DIR}/storages/python/python-storage-plugins
  )
list(REMOVE_DUPLICATES dlite_PYTHON_STORAGE_PLUGINS)

# DLITE_PYTHON_MAPPING_PLUGIN_DIRS - search path for Python mapping plugins
set(dlite_PYTHON_MAPPING_PLUGINS
  ${dlite_SOURCE_DIR}/bindings/python/python-mapping-plugins
  )
list(REMOVE_DUPLICATES dlite_PYTHON_MAPPING_PLUGINS)

# DLITE_TEMPLATE_DIRS - search path for DLite templates
set(dlite_TEMPLATES
  ${dlite_SOURCE_DIR}/tools/templates
  )
list(REMOVE_DUPLICATES dlite_TEMPLATES)

# DLITE_STORAGES - DLite storages (inc. metadata)
set(dlite_STORAGES
  ${dlite_SOURCE_DIR}/examples/storages/*.json
  )
list(REMOVE_DUPLICATES dlite_STORAGES)

#if(UNIX)
#  string(REPLACE ";" ":" dlite_PATH             "${dlite_PATH}")
#  string(REPLACE ";" ":" dlite_LD_LIBRARY_PATH  "${dlite_LD_LIBRARY_PATH}")
#  string(REPLACE ";" ":" dlite_PYTHONPATH       "${dlite_PYTHONPATH}")
#  string(REPLACE ";" ":" dlite_STORAGE_PLUGINS  "${dlite_STORAGE_PLUGINS}")
#  string(REPLACE ";" ":" dlite_MAPPING_PLUGINS  "${dlite_MAPPING_PLUGINS}")
#  string(REPLACE ";" ":" dlite_PYTHON_STORAGE_PLUGINS  "${dlite_PYTHON_STORAGE_PLUGINS}")
#  string(REPLACE ";" ":" dlite_PYTHON_MAPPING_PLUGINS  "${dlite_PYTHON_MAPPING_PLUGINS}")
#  string(REPLACE ";" ":" dlite_TEMPLATES        "${dlite_TEMPLATES}")
#endif()

make_platform_paths(
  PATHS
    dlite_LD_LIBRARY_PATH
    dlite_PYTHONPATH
    dlite_PYTHON_STORAGE_PLUGINS
    dlite_PYTHON_MAPPING_PLUGINS
    dlite_TEMPLATES
    dlite_STORAGES
  MULTI_CONFIG_PATHS
    dlite_PATH
    dlite_STORAGE_PLUGINS
    dlite_MAPPING_PLUGINS
  #PATHSEP0
  )

# Note, DLITE_STORAGES uses "|" as separator on all systems
string(REPLACE ";" "|" dlite_STORAGES         "${dlite_STORAGES}")
string(REPLACE ";" "|" dlite_STORAGES_NATIVE  "${dlite_STORAGES_NATIVE}")
string(REPLACE ";" "|" dlite_STORAGES_UNIX    "${dlite_STORAGES_UNIX}")
string(REPLACE ";" "|" dlite_STORAGES_WINDOWS "${dlite_STORAGES_WINDOWS}")


# Add target "show" for showing test environment
set(test_env
  "========== Test environment ========="
  "export PATH=${dlite_PATH}"
  "export LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}"
  "export PYTHONPATH=${dlite_PYTHONPATH}"
  "export DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}"
  "export DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}"
  "export DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}"
  "export DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}"
  "export DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}"
  "export DLITE_STORAGES='${dlite_STORAGES}'"
  ""
  "========== Python =================="
  "Python3_LIBRARIES:          ${Python3_LIBRARIES}"
  "Python3_LIBRARY_DIRS:       ${Python3_LIBRARY_DIRS}"
  "Python3_INCLUDE_DIRS:       ${Python3_INCLUDE_DIRS}"
  "Python3_NumPy_INCLUDE_DIRS: ${Python3_NumPy_INCLUDE_DIRS}"
  "Python3_NumPy_LIBRARY_DIRS: ${Python3_NumPy_LIBRARY_DIRS}"
  ""
  "========== Jansson =================="
  "JANSSON_INCLUDE_DIRS: ${JANSSON_INCLUDE_DIRS}"
  "JANSSON_LIBRARIES:    ${JANSSON_LIBRARIES}"
  "JANSSON_LIBRARY_DIR:  ${JANSSON_LIBRARY_DIR}"
  "JANSSON_RUNTIME_DIR:  ${JANSSON_RUNTIME_DIR}"
  ""
  "========== HDF5 ====================="
  "HDF5_INCLUDE_DIRS:    ${HDF5_INCLUDE_DIRS}"
  "HDF5_LIBRARIES:       ${HDF5_LIBRARIES}"
  "HDF5_LIBRARY_DIR:     ${HDF5_LIBRARY_DIR}"
  "HDF5_RUNTIME_DIR:     ${HDF5_RUNTIME_DIR}"
  "====================================="
  )
set(cmd "")
foreach(line ${test_env})
  list(APPEND cmd COMMAND ${CMAKE_COMMAND} -E echo "${line}")
endforeach()
add_custom_target(show ${cmd})

#################################################################


# Subdirectories
add_subdirectory(src)

# Storage plugins
if(WITH_HDF5)
  add_subdirectory(storages/hdf5)
endif()
if(WITH_JSON)
  add_subdirectory(storages/json)
endif()
if(HAVE_REDLAND)
  add_subdirectory(storages/rdf)
endif()

if(WITH_PYTHON)
  if(NOT WITH_JSON)
    message(FATAL_ERROR "Python bindings and testing depend on the json-plugin, set WITH_JSON=ON and eventually BUILD_JSON=ON")
  endif()
  add_subdirectory(storages/python)
  add_subdirectory(bindings/python)
endif()

# Tools - may depend on storage plugins
add_subdirectory(tools)

# Fortran - depends on tools
if(WITH_FORTRAN)
  add_subdirectory(bindings/fortran)
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

# CPack
# -----
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "${dlite_VENDOR}")
set(CPACK_PACKAGE_VERSION_MAJOR "${dlite_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${dlite_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${dlite_VERSION_RELEASE}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${dlite_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION "${dlite_LONG_DESCRIPTION}")
#set(CPACK_RESOURCE_FILE_WELCOME "")
set(CPACK_PACKAGE_CONTACT "${dlite_CONTACT}")
if(CROSS_TARGET)
  set(arch "${CROSS_TARGET}")
else()
  set(arch "${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_PACKAGE_FILE_NAME "dlite-${dlite_VERSION}-${arch}")
set(CPACK_PACKAGE_ICON "${dlite_SOURCE_DIR}/doc/figs/logo-circle.ico")
set(CPACK_RESOURCE_FILE_LICENSE "${dlite_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${dlite_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_SOURCE_IGNORE_FILES}
  ".git/*" "${dlite_BINARY_DIR}/*" "build*/*" "dist/*" "dlite.egg-info/*")

# DEB package
set(CPACK_DEBIAN_PACKAGE_DEPENDS
  libhdf5
  libjansson
  python3
  python3-numpy
  )

# RPM package
set(CPACK_RPM_PACKAGE_LICENSE "${dlite_LICENSE}")
set(CPACK_RPM_PACKAGE_REQUIRES
  hdf5
  jansson
  python3
  python3-numpy
  )

# Include CPack after configuring it...
include(CPack)


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

configure_package_config_file(
  cmake/dliteConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/dliteConfig.cmake"
  INSTALL_DESTINATION "${DLITE_INSTALL_CMAKE_DIR}"
  PATH_VARS ${DLITE_CONFIG_VARS}
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
    "${dlite_SOURCE_DIR}/cmake/dliteCodeGen.cmake"
    "${dlite_SOURCE_DIR}/cmake/dliteCodeGen.bat.in"
    "${dlite_SOURCE_DIR}/cmake/dliteGenEnv.cmake"
    "${dlite_SOURCE_DIR}/cmake/dliteGenEnv-sub.cmake"
    "${dlite_SOURCE_DIR}/cmake/MakePlatformPaths.cmake"
  DESTINATION "${DLITE_INSTALL_CMAKE_DIR}"
  )
install(
  FILES "${dlite_SOURCE_DIR}/cmake/Finddlite.cmake"
  DESTINATION "share/cmake/Modules"
  )

if(CROSS_TARGET)
  # When cross-compiling, include the python in the distribution
  set(pyver "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
  install(
    FILES
      ${TOOLCHAIN_BINDIR}/python${pyver}.exe
      ${TOOLCHAIN_BINDIR}/libpython${pyver}.dll
      ${TOOLCHAIN_BINDIR}/libwinpthread-1.dll
      ${TOOLCHAIN_BINDIR}/libssp-0.dll
    DESTINATION "bin"
    )
  install(
    DIRECTORY ${TOOLCHAIN_LIBDIR}/python${pyver}
    DESTINATION "lib"
    )
endif()
