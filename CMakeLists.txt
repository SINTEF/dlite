# -*- Mode: cmake -*-
#
cmake_minimum_required(VERSION 3.14)

project(dlite
  VERSION 0.5.16
  DESCRIPTION "Lightweight data-centric framework for semantic interoperability"
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
option(WITH_PYTHON      "Whether to build Python 3 bindings"               YES)
option(WITH_FORTRAN     "Whether to build Fortran bindings"                 NO)
option(WITH_HDF5        "Whether to build with HDF5 support"               YES)
option(WITH_REDLAND     "Whether to build with RDF support (using librdf)" YES)
option(WITH_THREADS     "Whether to build with threading support"           NO)
option(WITH_DOC         "Whether to build documentation (sphinx+doxygen)"   NO)
option(WITH_EXAMPLES    "Whether to build/run examples during testing"     YES)
option(FORCE_EXAMPLES   "Whether to force building/running examples"        NO)
option(ALLOW_WARNINGS   "Whether to not fail on compilation warnings"      YES)
option(WITH_STATIC_PYTHON "Whether to compile with static python libraries" NO)

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

if(MSVC)
  # Fix utf-8 handling for MSVC: Force utf-8 encoded source-code files
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

  # Do not complain about strdup() and similar POSIX functions to be
  # deprecated in MSVC
  add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE")
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
if(WITH_HDF5)
  list(APPEND DLITE_CONFIG_VARS HDF5_LIBRARIES)
endif()
if(WITH_PYTHON)
  #FIME: Python3_LIBRARIES is undefined here
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


#
# hdf5
# ====
if(WITH_HDF5)
  find_package(HDF5 1.8.20 COMPONENTS C REQUIRED)
  if(WIN32)
    list(APPEND HDF5_LIBRARIES Kernel32)
  else()
    list(APPEND HDF5_LIBRARIES m dl)
  endif()
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
  message(STATUS "Looking for Python3")
  if(CROSS_TARGET AND MINGW)
    set(Python3_FIND_REGISTRY NEVER)
    set(Python3_FIND_STRATEGY LOCATION)
    set(Python3_ROOT_DIR /usr/${TOOLCHAIN_PREFIX})
  endif()

  if(WIN32)
    set(Python3_FIND_REGISTRY NEVER)
    set(Python3_FIND_STRATEGY LOCATION)
    set(Python3_USE_STATIC_LIBS TRUE)
  endif()

  #
  if(DEFINED ENV{VIRTUAL_ENV})
    message(STATUS "Detected virtual environment $ENV{VIRTUAL_ENV}")

    if(NOT Python3_FIND_VIRTUALENV)
      message(STATUS "Setting Python3_FIND_VIRTUALENV=ONLY")
      set(Python3_FIND_VIRTUALENV=ONLY)
    endif()

    if(NOT PYTHON_VERSION)
      find_program(
        python_exe
        NAMES python python3
        PATHS $ENV{VIRTUAL_ENV}/bin
        REQUIRED
        NO_DEFAULT_PATH
      )
      execute_process(
        COMMAND ${python_exe} -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}', end='')"
        OUTPUT_VARIABLE pyversion
      )
      message(STATUS "Setting PYTHON_VERSION=${pyversion}")
      set(PYTHON_VERSION "${pyversion}")
    endif()
  endif()

  # Find Python package
  set(CMAKE_CROSSCOMPILING_EMULATOR ${RUNNER})
  if (PYTHON_VERSION)
    find_package(Python3 ${PYTHON_VERSION} EXACT
      REQUIRED COMPONENTS Interpreter Development NumPy)
  else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
  endif()
  unset(CMAKE_CROSSCOMPILING_EMULATOR)

  # Link libraries when compiling against static Python (e.g. manylinux)
  if(WITH_STATIC_PYTHON)
    execute_process(
      COMMAND ${Python3_EXECUTABLE}-config --ldflags
      OUTPUT_VARIABLE Python3_LDFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    # TODO: find a more portable way to add the "-Xlinker -export-dynamic" options.
    # They are needed to ensure that the linker include all symbols in
    # the static Python library.
    set(Python3_STATIC_LIBS
      ${Python3_LDFLAGS}
      -Xlinker -export-dynamic
      ${Python3_LIBRARY}
      )
  endif()

  message(STATUS "Python3_LIBRARIES = ${Python3_LIBRARIES}")
  message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")
  message(STATUS "Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
  message(STATUS "Python3_NumPy_INCLUDE_DIRS = ${Python3_NumPy_INCLUDE_DIRS}")
  message(STATUS "Python3_STATIC_LIBS = ${Python3_STATIC_LIBS}")


  if (WIN32)
    # Prepare to workaround Anaconda issues on Windows
    # https://github.com/pytorch/pytorch/issues/17051
    # https://github.com/ContinuumIO/anaconda-issues/issues/11374
    include(FindPythonAnaconda)
    if (PYTHON_IS_ANACONDA)
      if (PYTHON_IS_ANACONDA_BASE)
        message(STATUS "Check Anaconda on Windows: Python3 is Anaconda - Root environment.")
      else()
        message(STATUS "Check Anaconda on Windows: Python3 is Anaconda - Virtual Environment.")
      endif()
    else()
      message(STATUS "Check Anaconda on Windows: Python3 is NOT Anaconda.")
    endif()
  endif()

  # On mingw, python depends on libffi
  find_library(ffi ffi)
  mark_as_advanced(ffi)
  get_filename_component(dlite_PATH_EXTRA ${ffi} DIRECTORY)

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
if(WITH_PYTHON)
  set(logfile "${CMAKE_CURRENT_BINARY_DIR}/valgrind_python.log")
  set(suppfile "${CMAKE_CURRENT_BINARY_DIR}/valgrind_python.supp")
  set(dlite_supp "${dlite_SOURCE_DIR}/cmake/valgrind-dlite.supp")
  set(python_supp "${dlite_SOURCE_DIR}/cmake/valgrind-python.supp")
else()
  set(logfile "${CMAKE_CURRENT_BINARY_DIR}/valgrind.log")
  set(suppfile "${dlite_SOURCE_DIR}/cmake/valgrind-dlite.supp")
endif()

# Must be assigned before including CTest
set(MEMORYCHECK_COMMAND_OPTIONS "-q --tool=memcheck --leak-check=yes --show-leak-kinds=definite,indirect --errors-for-leak-kinds=definite --num-callers=200 --keep-debuginfo=yes --suppressions=${suppfile} --gen-suppressions=all")

include(CTest)

find_program(MEMORYCHECK_COMMAND NAMES valgrind)

if(MEMORYCHECK_COMMAND)
  if(WITH_PYTHON)
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
      MAIN_DEPENDENCY ${dlite_supp}
      DEPENDS
        ${python_supp}
        ${dlite_SOURCE_DIR}/cmake/valgrind_python.py
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
  endif()

  set(MEMORYCHECK_SUPPRESSIONS_FILE
    ${suppfile}
    CACHE FILEPATH "File that contains suppressions for the memory checker"
    )

  add_custom_target(memcheck
    COMMAND ${CMAKE_COMMAND}
        -E env PYTHONMALLOC=malloc ${CMAKE_CTEST_COMMAND}
            --exclude-regex static-code-analysis
            --force-new-ctest-process
            --output-on-failure
            --test-action memcheck
    DEPENDS ${suppfile}
    COMMENT "Runs memory check with valgrind"
    )
endif()


find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_test(
    NAME static-code-analysis
    COMMAND ${CPPCHECK}
      --language=c -q --force --error-exitcode=2 --inline-suppr
      -i build -i python
      -I${dlite_SOURCE_DIR}/src ${NUMBER_OF_CPUS_OPTION}
      ${dlite_SOURCE_DIR}/src
      ${dlite_SOURCE_DIR}/storages
      ${dlite_SOURCE_DIR}/tools
      ${dlite_SOURCE_DIR}/bindings
      ${dlite_SOURCE_DIR}/examples
      ${dlite_BINARY_DIR}/src
      ${dlite_BINARY_DIR}/storates
      ${dlite_BINARY_DIR}/tools
      ${dlite_BINARY_DIR}/bindings
      #${dlite_BINARY_DIR}/examples
    )
endif()



# -------------------------------------------
# Environment for running tests in build tree
# -------------------------------------------

# Name of all targets that produce a DLL that we have to take special
# care of on Windows
set(dll_targets
  dlite
  dlite-utils
  dlite-plugins-json
  dlite-plugins-python
  )
if(WITH_HDF5)
  list(APPEND targets dlite-plugins-hdf5)
endif()
if(WITH_REDLAND)
  list(APPEND targets dlite-plugins-rdf)
endif()

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

# Directory and path separators
if(WIN32)
  set(DIRSEP "\\")
  set(PATHSEP ";")
else()
  set(DIRSEP "/")
  set(PATHSEP ":")
endif()

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
    list(APPEND dlite_PATH ${Python3_RUNTIME_LIBRARY_DIRS})
    list(APPEND dlite_PATH ${Python3_NumPy_LIBRARY_DIRS})
    if (WIN32 AND PYTHON_IS_ANACONDA)
      # Fix dll search path for Anaconda on Windows
      list(APPEND dlite_PATH "${Python3_RUNTIME_LIBRARY_DIRS}/Library/bin")
    endif()
  endif()
  if(WITH_FORTRAN)
    build_append(dlite_PATH ${dlite_BINARY_DIR}/bindings/fortran)
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
build_append(dlite_STORAGE_PLUGINS ${dlite_BINARY_DIR}/storages/json)
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
  )

# Workaround for cross-compiling for Windows on Linux using MinGW.
# Some paths in ${dlite_PATH} may start with "Z:/" which the
# $<SHELL_PATH:...> generator expression complains about being a
# relative path.  To fix this, we strip off the initial "Z:".
if(CROSS_TARGET AND MINGW)
  set(paths "")
  foreach(path ${dlite_PATH})
    string(REGEX REPLACE "^[A-Z]:(/.*)" "\\1" out ${path})
    list(APPEND paths ${out})
  endforeach()
  set(dlite_PATH ${paths})
endif()


set(dlite_PATH_NATIVE1 "${CMAKE_INSTALL_PREFIX}/bin")
foreach(path ${dlite_PATH})
  list(APPEND dlite_PATH_NATIVE1 $<SHELL_PATH:${path}>)
endforeach()
if(WIN32)
  string(REPLACE ";" "\\\\;" dlite_PATH_NATIVE "${dlite_PATH_NATIVE1}")
else()
  string(REPLACE ";" ":" dlite_PATH_NATIVE "${dlite_PATH_NATIVE1}")
endif()

# Note, DLITE_STORAGES uses "|" as separator on all systems
string(REPLACE ";" "|" dlite_STORAGES         "${dlite_STORAGES}")
string(REPLACE ";" "|" dlite_STORAGES_NATIVE  "${dlite_STORAGES_NATIVE}")
string(REPLACE ";" "|" dlite_STORAGES_UNIX    "${dlite_STORAGES_UNIX}")
string(REPLACE ";" "|" dlite_STORAGES_WINDOWS "${dlite_STORAGES_WINDOWS}")


# Add target "show" for showing test environment
set(test_env
  "========== Test environment ========="
  "export DLITE_ROOT=${DLITE_ROOT}"
  "export PATH=${dlite_PATH}"
  "dlite_PATH_NATIVE=${dlite_PATH_NATIVE}"
  "export LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}"
  "export PYTHONPATH=${dlite_PYTHONPATH}"
  "PYTHONPATH_NATIVE=${dlite_PYTHONPATH_NATIVE}"
  "export DLITE_STORAGE_PLUGIN_DIRS=${dlite_STORAGE_PLUGINS}"
  "export DLITE_MAPPING_PLUGIN_DIRS=${dlite_MAPPING_PLUGINS}"
  "export DLITE_PYTHON_STORAGE_PLUGIN_DIRS=${dlite_PYTHON_STORAGE_PLUGINS}"
  "export DLITE_PYTHON_MAPPING_PLUGIN_DIRS=${dlite_PYTHON_MAPPING_PLUGINS}"
  "export DLITE_TEMPLATE_DIRS=${dlite_TEMPLATES}"
  "export DLITE_STORAGES='${dlite_STORAGES}'"
  ""
  "========== Python ==================="
  "Python3_LIBRARIES:          ${Python3_LIBRARIES}"
  "Python3_LIBRARY_DIRS:       ${Python3_LIBRARY_DIRS}"
  "Python3_INCLUDE_DIRS:       ${Python3_INCLUDE_DIRS}"
  "Python3_NumPy_INCLUDE_DIRS: ${Python3_NumPy_INCLUDE_DIRS}"
  "Python3_NumPy_LIBRARY_DIRS: ${Python3_NumPy_LIBRARY_DIRS}"
  ""
  "========== Redland =================="
  "REDLAND_INCLUDE_DIRS: ${REDLAND_INCLUDE_DIRS}"
  "REDLAND_LIBRARIES:    ${REDLAND_LIBRARIES}"
  "REDLAND_LIBRARY_DIR:  ${REDLAND_LIBRARY_DIR}"
  "REDLAND_RUNTIME_DIR:  ${REDLAND_RUNTIME_DIR}"
  ""
  "RASQAL_INCLUDE_DIRS: ${RASQAL_INCLUDE_DIRS}"
  "RASQAL_LIBRARIES:    ${RASQAL_LIBRARIES}"
  "RASQAL_DEFINITIONS:  ${RASQAL_DEFINITIONS}"
  ""
  "RAPTOR_INCLUDE_DIRS: ${RAPTOR_INCLUDE_DIRS}"
  "RAPTOR_LIBRARIES:    ${RAPTOR_LIBRARIES}"
  "RAPTOR_DEFINITIONS:  ${RAPTOR_DEFINITIONS}"
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
add_subdirectory(storages/json)
if(WITH_HDF5)
  add_subdirectory(storages/hdf5)
endif()
if(HAVE_REDLAND)
  add_subdirectory(storages/rdf)
endif()
if(WITH_PYTHON)
  add_subdirectory(bindings/python)
  add_subdirectory(storages/python)
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
set(CPACK_RESOURCE_FILE_LICENSE "${dlite_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${dlite_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_SOURCE_IGNORE_FILES}
  ".git/*" "${dlite_BINARY_DIR}/*" "build*/*" "dist/*" "dlite.egg-info/*")

# DEB package
set(CPACK_DEBIAN_PACKAGE_DEPENDS
  "python3, python3-numpy, librdf0, libraptor2-dev, libhdf5-dev")

# RPM package
set(CPACK_RPM_PACKAGE_LICENSE "${dlite_LICENSE}")
set(CPACK_RPM_PACKAGE_REQUIRES
  "python3, python3-numpy, redland, raptor2, hdf5")

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
    LICENSE
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

# Patch virtualenv activate.sh script
if(DEFINED ENV{VIRTUAL_ENV})
  find_program(BASH bash)
  find_program(PATCH patch)
  install(
    CODE "execute_process(
              COMMAND ${BASH} ${dlite_SOURCE_DIR}/cmake/patch-activate.sh
              OUTPUT_QUIET
         )"
  )
  install(
    PROGRAMS ${dlite_SOURCE_DIR}/cmake/patch-activate.sh
    TYPE BIN
  )
endif()


# When cross-compiling, include Python binaries in the distribution
if(CROSS_TARGET AND HAVE_PYTHON)
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


# Uninstall
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND}
      -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    )
endif()
